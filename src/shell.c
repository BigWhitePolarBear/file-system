#include "shell.h"
#include "common.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "termios.h"
#include "unistd.h"

int main()
{
    if (open_shm())
    {
        printf("开启共享内存失败，退出系统！\n");
        return -1;
    }

    msg_t msg;
    uint32_t uid;
    printf("请输入用户 id ：");
    scanf("%u", &uid);
    if (uid >= MAX_USER_CNT)
    {
        printf("非法用户 id ！\n");
        return -1;
    }
    printf("请输入用户密码：");
    char pwd[PWD_LEN];
    scanf("%s", pwd);
    msg.session_id = uid;
    strcpy(msg.cmd, "LOGIN ");
    strcpy(msg.cmd + 6, pwd);
    // 先发送 id 和密码完成登录。
    sem_wait(mutex);
    memcpy(shm, &msg, SHM_SIZE);
    sem_post(in_ready);
    sem_wait(out_ready);
    memcpy(&msg, shm, SHM_SIZE);
    if (strncmp(msg.cmd, "SUCCESS", 7))
    {
        printf("密码错误或登录会话过多！\n");
        sem_post(mutex);
        return -1;
    }

    // 开启输出共享内存和输出通知信号量。
    char spec_shm_name[TIMESTAMP_LEN + 4];
    spec_shm_name[TIMESTAMP_LEN + 3] = 0;
    strcpy(spec_shm_name, "fs_");
    strncpy(spec_shm_name + 3, msg.cmd + 8, TIMESTAMP_LEN);
    int fd = shm_open(spec_shm_name, O_CREAT | O_RDWR, 0662);
    if (fd == -1)
    {
        printf("开启专属共享内存链接失败！\n");
        return -1;
    }
    if (ftruncate(fd, SPEC_SHM_SIZE))
    {
        printf("为专属共享内存分配空间失败！\n");
        return -1;
    }
    spec_shm = mmap(NULL, SPEC_SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (*(int *)spec_shm == -1)
    {
        printf("映射专属共享内存失败！\n");
        return -1;
    }

    char sepc_sem_out_ready_name[TIMESTAMP_LEN + 14];
    sepc_sem_out_ready_name[TIMESTAMP_LEN + 13] = 0;
    strcpy(sepc_sem_out_ready_name, "fs_out_ready_");
    strncpy(sepc_sem_out_ready_name + 13, msg.cmd + 8, TIMESTAMP_LEN);
    spec_out_ready = sem_open(sepc_sem_out_ready_name, O_CREAT | O_EXCL, OUT_SEM_PERM, 0);
    if (spec_out_ready == SEM_FAILED)
    {
        printf("开启专属输出通知信号量失败！\n");
        return -1;
    }
    // 通知后端
    sem_post(in_ready);

    // 检查后端是否也成功开启信号量
    sem_wait(out_ready);
    memcpy(&msg, shm, SHM_SIZE);
    if (strncmp(msg.cmd, "SUCCESS AGAIN", 13))
    {
        printf("后端初始化输出共享内存或信号量时出错！\n");

        if (shm_unlink(spec_shm_name))
            printf("断开输出共享内存链接失败！\n");
        if (sem_unlink(sepc_sem_out_ready_name))
            printf("断开输出通知信号量链接失败！\n");

        sem_post(mutex);
        return -1;
    }
    sem_post(mutex);

    struct termios tm, tm_old;
    fd = 0;
    // 保存当前终端设置
    if (tcgetattr(fd, &tm) < 0)
    {
        printf("获取当前终端状态失败！\n");
        return -1;
    }
    tm_old = tm;
    cfmakeraw(&tm); // 原始模式
    if (tcsetattr(fd, TCSANOW, &tm) < 0)
    {
        printf("设置终端状态为原始模式失败！\n");
        return -1;
    }

    // 初始化终端缓冲
    head = malloc(sizeof(inbuf_t));
    memset(head, 0, sizeof(inbuf_t));
    tail = malloc(sizeof(inbuf_t));
    memset(tail, 0, sizeof(inbuf_t));
    cur = malloc(sizeof(inbuf_t));
    memset(cur, 0, sizeof(inbuf_t));
    head->nxt = cur;
    tail->pre = cur;
    cur->pre = head;
    cur->nxt = tail;

    // 丢弃上一个换行符。
    getchar();
    while (1)
    {
        // 获取输入并传输到后端。
        printf(">> ");
        int ret = handle_input(&msg);
        if (ret == 0)
            continue;
        if (!strcmp(msg.cmd, "q") || !strcmp(msg.cmd, "exit") || !strcmp(msg.cmd, "quit"))
            break;
        sem_wait(mutex);
        memcpy(shm, &msg, SHM_SIZE);
        sem_post(in_ready);
        sem_wait(out_ready);
        sem_post(mutex);
        if (ret == -1) // shutdown
            goto Exit;

        // 等待后端返回输出。
        sem_wait(spec_out_ready);
        if (!strncmp(spec_shm, "ERROR", 5))
            printf("ERROR\r\n");
        else
            printf("%s", (char *)spec_shm);
        if (strlen((char *)spec_shm))
            printf("\r\n");
    }

    // 发送登出消息。
    strcpy(msg.cmd, "LOGOUT");
    sem_wait(mutex);
    memcpy(shm, &msg, SHM_SIZE);
    sem_post(in_ready);
    sem_wait(out_ready);
    sem_post(mutex);

Exit:
    int ret = 0;
    // 恢复原始终端状态。
    if (tcsetattr(fd, TCSANOW, &tm_old) < 0)
    {
        printf("恢复原始终端状态失败！\r\n");
        ret = -1;
    }

    if (shm_unlink(spec_shm_name))
    {
        printf("断开输出共享内存链接失败！\n");
        ret = -1;
    }
    if (sem_unlink(sepc_sem_out_ready_name))
    {
        printf("断开输出通知信号量链接失败！\n");
        ret = -1;
    }
    return ret;
}

int open_shm()
{
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        printf("开启共享内存链接失败！\n");
        return -1;
    }
    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*(int *)shm == -1)
    {
        printf("映射共享内存失败！\n");
        return -1;
    }

    mutex = sem_open(SEM_MUTEX_NAME, O_EXCL);
    if (mutex == SEM_FAILED)
    {
        printf("开启互斥信号量失败！\n");
        return -1;
    }
    in_ready = sem_open(SEM_IN_READY_NAME, O_EXCL);
    if (in_ready == SEM_FAILED)
    {
        printf("开启输入通知信号量失败！\n");
        return -1;
    }
    out_ready = sem_open(SEM_OUT_READY_NAME, O_EXCL, IN_SEM_PERM, 0);
    if (out_ready == SEM_FAILED)
    {
        printf("开启输出通知信号量失败！\n");
        return -1;
    }

    return 0;
}

int handle_input(msg_t *msg)
{
    memset(msg->cmd, 0, CMD_LEN);
    char c;
    inbuf_t *p = cur;
    while (1)
    {
        if (c == '\r')
        {
            c = 0;
            break;
        }
        c = getchar();
        switch (c)
        {
        case 27: // ↑ ↓ ← →
            getchar();
            c = getchar();
            switch (c)
            {
            case 65: // ↑
                if (head->nxt == p)
                    continue;
                for (uint8_t i = 0; i < cur->len; i++)
                    putchar('\b');
                for (uint8_t i = 0; i < cur->len; i++)
                    putchar(' ');
                for (uint8_t i = 0; i < cur->len; i++)
                    putchar('\b');
                p = p->pre;
                strcpy(cur->cmd, p->cmd);
                cur->len = p->len;
                memset(cur->cmd + cur->len, 0, CMD_LEN - cur->len);
                printf("%s", cur->cmd);
                continue;
            case 66: // ↓
                if (tail->pre == p)
                    continue;
                for (uint8_t i = 0; i < cur->len; i++)
                    putchar('\b');
                for (uint8_t i = 0; i < cur->len; i++)
                    putchar(' ');
                for (uint8_t i = 0; i < cur->len; i++)
                    putchar('\b');
                p = p->nxt;
                strcpy(cur->cmd, p->cmd);
                cur->len = p->len;
                memset(cur->cmd + cur->len, 0, CMD_LEN - cur->len);
                printf("%s", cur->cmd);
                continue;
            }
            continue;

        case '\r': // 换行
            printf("\r\n");
            continue;

        case '\b': // 退格
            if (cur->len == 0)
                continue;
            cur->cmd[--(cur->len)] = '\0';
            printf("\b \b");
            break;

        default:
            if (cur->len == CMD_LEN - 1)
                continue;
            putchar(c);
            cur->cmd[(cur->len)++] = c;
        }
    }
    if (cur->len == 0)
        return 0;

    strcpy(msg->cmd, cur->cmd);
    tail->nxt = malloc(sizeof(inbuf_t));
    memset(tail->nxt, 0, sizeof(inbuf_t));
    cur = tail;
    tail = tail->nxt;
    tail->pre = cur;

    if (cmdline_cnt++ == MAX_CMD_LINE)
    {
        inbuf_t *t = head;
        head = head->nxt;
        free(t);
    }

    if (session_id2uid(msg->session_id) == 0 && !strncmp(msg->cmd, "shutdown", 8))
        return -1;

    return cur->pre->len;
}

uint32_t session_id2uid(uint32_t session_id)
{
    return session_id / MAX_SESSION_PER_USER;
}