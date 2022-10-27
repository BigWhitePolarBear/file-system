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

    inmsg_t inmsg;
    printf("请输入用户 id ：");
    scanf("%u", &inmsg.uid);
    if (inmsg.uid >= MAX_USER_CNT)
    {
        printf("非法用户 id ！\n");
        return -1;
    }
    printf("请输入用户密码：");
    char pwd[PWD_LEN];
    scanf("%s", pwd);
    strcpy(inmsg.cmd, "LOGIN ");
    strcpy(inmsg.cmd + 6, pwd);
    // 先发送 id 和密码完成登录。
    memcpy(in_shm, &inmsg, IN_MSG_SIZE);
    sem_wait(in_mutex);
    sem_post(in_ready);
    usleep(SYNC_WAIT);
    sem_wait(in_ready);
    memcpy(&inmsg, in_shm, IN_MSG_SIZE);
    if (strncmp(inmsg.cmd, "SUCCESS", 7))
    {
        printf("密码错误！\n");
        return -1;
    }
    // 开启输出共享内存。
    char out_shm_name[TIMESTAMP_LEN + 8];
    out_shm_name[TIMESTAMP_LEN + 7] = 0;
    strcpy(out_shm_name, "fs_out_");
    strncpy(out_shm_name + 7, inmsg.cmd + 8, TIMESTAMP_LEN);
    int fd = shm_open(out_shm_name, O_CREAT | O_RDWR, 0662);
    ftruncate(fd, OUT_BUF_SIZE);
    out_shm = mmap(NULL, OUT_BUF_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    sem_post(in_ready);
    usleep(SYNC_WAIT);
    sem_wait(in_ready);
    sem_post(in_mutex);

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
    tail = malloc(sizeof(inbuf_t));
    cur = malloc(sizeof(inbuf_t));
    head->nxt = cur;
    tail->pre = cur;
    cur->pre = head;
    cur->nxt = tail;

    // 丢弃上一个换行符。
    getchar();
    while (1)
    {
        printf(">> ");
        if (handle_input(&inmsg) == 0)
            continue;
        printf("%s\r\n", inmsg.cmd);
        if (!strcmp(inmsg.cmd, "q") || !strcmp(inmsg.cmd, "exit") || !strcmp(inmsg.cmd, "quit"))
            break;
        memcpy(in_shm, &inmsg, IN_MSG_SIZE);
        sem_wait(in_mutex);
        sem_post(in_ready);
        usleep(SYNC_WAIT);
        sem_wait(in_ready);
        sem_post(in_mutex);
    }

    // 恢复原始终端状态。
    if (tcsetattr(fd, TCSANOW, &tm_old) < 0)
    {
        printf("恢复原始终端状态失败！\n");
        return -1;
    }

    // 发送退出登录的消息。
    strcpy(inmsg.cmd, "LOGOUT");
    memcpy(in_shm, &inmsg, IN_MSG_SIZE);
    sem_wait(in_mutex);
    sem_post(in_ready);
    usleep(SYNC_WAIT);
    sem_wait(in_ready);
    sem_post(in_mutex);

    if (shm_unlink(out_shm_name))
    {
        printf("断开输出共享内存链接失败！\n");
        return -1;
    }

    return 0;
}

int open_shm()
{
    int fd = shm_open(IN_SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        printf("开启共享内存链接失败！\n");
        return -1;
    }
    in_shm = mmap(NULL, IN_MSG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*(int *)in_shm == -1)
    {
        printf("映射共享内存失败！\n");
        return -1;
    }

    in_mutex = sem_open(IN_SEM_MUTEX_NAME, O_EXCL);
    if (in_mutex == SEM_FAILED)
    {
        printf("开启输入互斥信号量失败！\n");
        return -1;
    }
    in_ready = sem_open(IN_SEM_READY_NAME, O_EXCL);
    if (in_ready == SEM_FAILED)
    {
        printf("开启输入通知信号量失败！\n");
        return -1;
    }

    return 0;
}

int handle_input(inmsg_t *inmsg)
{
    inmsg->cmd[0] = '\0';
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
        case -1: // 没有输入
            continue;

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
            cur->cmd[--cur->len] = '\0';
            printf("\b \b");
            break;

        default:
            if (cur->len == CMD_LEN - 1)
                continue;
            putchar(c);
            cur->cmd[cur->len++] = c;
        }
    }
    if (cur->len == 0)
        return 0;
    strcpy(inmsg->cmd, cur->cmd);
    tail->nxt = malloc(sizeof(inbuf_t));
    cur = tail;
    tail = tail->nxt;
    tail->pre = cur;

    if (cmdline_cnt++ == MAX_CMD_LINE)
    {
        inbuf_t *t = head;
        head = head->nxt;
        free(t);
    }

    return cur->pre->len;
}