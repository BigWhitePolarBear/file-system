#include "cmd.h"
#include "common.h"
#include "fcntl.h"
#include "fun.h"
#include "semaphore.h"
#include "stdio.h"
#include "string.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "time.h"
#include "unistd.h"

void *shm;

sem_t *mutex;
sem_t *in_ready;
sem_t *out_ready;

uint8_t logined[MAX_USER_CNT];
void *spec_shms[MAX_USER_CNT];
uint16_t last_spec_shm_pos[MAX_USER_CNT];
sem_t *spec_out_readys[MAX_USER_CNT];
uint32_t working_dirs[MAX_USER_CNT];

int open_shm()
{
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        printf("开启共享内存链接失败！\n");
        return -1;
    }
    if (ftruncate(fd, SHM_SIZE))
    {
        printf("为共享内存分配空间失败！\n");
        return -1;
    }
    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*(int *)shm == -1)
    {
        printf("映射共享内存失败！\n");
        return -1;
    }

    mutex = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 1);
    if (mutex == SEM_FAILED)
    {
        printf("开启互斥信号量失败！\n");
        return -1;
    }
    in_ready = sem_open(SEM_IN_READY_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 0);
    if (in_ready == SEM_FAILED)
    {
        printf("开启输入通知信号量失败！\n");
        return -1;
    }
    out_ready = sem_open(SEM_OUT_READY_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 0);
    if (out_ready == SEM_FAILED)
    {
        printf("开启输出通知信号量失败！\n");
        return -1;
    }

    return 0;
}

int unlink_shm()
{
    if (shm_unlink(SHM_NAME))
    {
        printf("断开共享内存链接失败！\n");
        return -1;
    }
    if (sem_unlink(SEM_MUTEX_NAME))
    {
        printf("断开互斥信号量链接失败！\n");
        return -1;
    }
    if (sem_unlink(SEM_IN_READY_NAME))
    {
        printf("断开输入通知信号量链接失败！\n");
        return -1;
    }
    if (sem_unlink(SEM_OUT_READY_NAME))
    {
        printf("断开输出通知信号量链接失败！\n");
        return -1;
    }
    return 0;
}

void handle_msg()
{
    msg_t msg;
    while (1)
    {
        if (sem_wait(in_ready))
        {
            printf("获取输入通知信号量失败！\n");
            return;
        }
        memcpy(&msg, shm, SHM_SIZE);

        printf("%s\n", msg.cmd);

        if (!logined[msg.uid] && !strncmp(msg.cmd, "LOGIN", 5))
        {
            // 登录信息。
            char pwd[PWD_LEN];
            strncpy(pwd, msg.cmd + 6, PWD_LEN);
            if (login(msg.uid, pwd)) // 密码正确
            {
                strcpy(msg.cmd, "SUCCESS");
                // 使用当前时间戳作为对应输出共享内存名字。
                sprintf(msg.cmd + 7, " %lu", get_timestamp());
                memcpy(shm, &msg, SHM_SIZE);

                // 等待客户端开启输出共享内存和输出通知信号量。
                sem_post(out_ready);
                sem_wait(in_ready);
                char spec_shm_name[TIMESTAMP_LEN + 4];
                spec_shm_name[TIMESTAMP_LEN + 3] = 0;
                strcpy(spec_shm_name, "fs_");
                strncpy(spec_shm_name + 3, msg.cmd + 8, TIMESTAMP_LEN);
                int fd = shm_open(spec_shm_name, O_RDWR, 0662);
                if (fd == -1)
                {
                    printf("连接专属共享内存失败！\n");
                    sem_post(out_ready);
                    continue;
                }
                spec_shms[msg.uid] = mmap(NULL, SPEC_SHM_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
                if (*(int *)spec_shms[msg.uid] == -1)
                {
                    printf("映射专属共享内存失败！\n");
                    sem_post(out_ready);
                    continue;
                }
                memset(spec_shms[msg.uid], 0, SPEC_SHM_SIZE);

                char spec_sem_out_ready_name[TIMESTAMP_LEN + 14];
                spec_sem_out_ready_name[TIMESTAMP_LEN + 13] = 0;
                strcpy(spec_sem_out_ready_name, "fs_out_ready_");
                strncpy(spec_sem_out_ready_name + 13, msg.cmd + 8, TIMESTAMP_LEN);
                spec_out_readys[msg.uid] = sem_open(spec_sem_out_ready_name, O_EXCL);
                if (spec_out_readys[msg.uid] == SEM_FAILED)
                {
                    spec_shms[msg.uid] = NULL;
                    printf("连接专属输出通知信号量失败！\n");
                    sem_post(out_ready);
                    continue;
                }

                // 再次通知 shell
                strcpy(msg.cmd, "SUCCESS AGAIN");
                memcpy(shm, &msg, SHM_SIZE);
                logined[msg.uid] = 1;
                working_dirs[msg.uid] = 0;
            }
            sem_post(out_ready);
        }
        else if (logined[msg.uid])
        {
            // 登出。
            if (!strncmp(msg.cmd, "LOGOUT", 6))
            {
                logined[msg.uid] = 0;
                working_dirs[msg.uid] = 0xffffffff;
                spec_shms[msg.uid] = NULL;
                sem_post(out_ready);
            }
            else if (msg.uid == 0 && (!strncmp(msg.cmd, "shutdown", 8)))
            {
                memset(spec_shms[msg.uid], 0, last_spec_shm_pos[msg.uid]);
                sem_post(out_ready);
                sem_post(spec_out_readys[msg.uid]);
                return;
            }
            else
            {
                sem_post(out_ready);
                handle_cmd(&msg);
                sem_post(spec_out_readys[msg.uid]);
            }
        }
    }
}

void handle_cmd(msg_t *msg)
{
    void *spec_shm = spec_shms[msg->uid];
    memset(spec_shm, 0, last_spec_shm_pos[msg->uid]);

    if (!strncmp(msg->cmd, "info", 4))
        last_spec_shm_pos[msg->uid] = info(msg->uid);
    else if (!strncmp(msg->cmd, "ls", 2) || !strncmp(msg->cmd, "dir", 3))
        if (!strncmp(msg->cmd, "ls -l", 5) || !strncmp(msg->cmd, "dir -s", 6))
            last_spec_shm_pos[msg->uid] = ls_detail(msg->uid);
        else
            last_spec_shm_pos[msg->uid] = ls(msg->uid);
    else
        last_spec_shm_pos[msg->uid] = unknown(msg->uid);

    return;
}