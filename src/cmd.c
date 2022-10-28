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

void *in_shm;

sem_t *in_mutex;
sem_t *in_ready;

uint8_t logined[MAX_USER_CNT];
void *out_shms[MAX_USER_CNT];
sem_t *out_readys[MAX_USER_CNT];

int open_shm()
{
    int fd = shm_open(IN_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        printf("开启输入共享内存链接失败！\n");
        return -1;
    }
    ftruncate(fd, IN_MSG_SIZE);
    in_shm = mmap(NULL, IN_MSG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*(int *)in_shm == -1)
    {
        printf("映射输入共享内存失败！\n");
        return -1;
    }

    in_mutex = sem_open(IN_SEM_MUTEX_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 1);
    if (in_mutex == SEM_FAILED)
    {
        printf("开启输入互斥信号量失败！\n");
        return -1;
    }
    in_ready = sem_open(IN_SEM_READY_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 0);
    if (in_ready == SEM_FAILED)
    {
        printf("开启输入通知信号量失败！\n");
        return -1;
    }

    return 0;
}

int unlink_shm()
{
    if (shm_unlink(IN_SHM_NAME))
    {
        printf("断开输入共享内存链接失败！\n");
        return -1;
    }
    if (sem_unlink(IN_SEM_MUTEX_NAME))
    {
        printf("断开输入互斥信号量链接失败！\n");
        return -1;
    }
    if (sem_unlink(IN_SEM_READY_NAME))
    {
        printf("断开输入通知信号量链接失败！\n");
        return -1;
    }
    return 0;
}

void handle_msg()
{
    inmsg_t inmsg;
    while (1)
    {
        if (sem_wait(in_ready))
        {
            printf("获取输入通知信号量失败！\n");
            return;
        }
        memcpy(&inmsg, in_shm, IN_MSG_SIZE);

        printf("%s\n", inmsg.cmd);

        if (!logined[inmsg.uid] && !strncmp(inmsg.cmd, "LOGIN", 5))
        {
            // 登录信息。
            char pwd[PWD_LEN];
            strncpy(pwd, inmsg.cmd + 6, PWD_LEN);
            if (login(inmsg.uid, pwd))
            {
                // 密码错误。
                inmsg.uid = 0xffffffff;
                memcpy(in_shm, &inmsg, IN_MSG_SIZE);
            }
            else
            {
                logined[inmsg.uid] = 1;
                strcpy(inmsg.cmd, "SUCCESS");
                // 使用当前时间戳作为对应输出共享内存名字。
                sprintf(inmsg.cmd + 7, " %lu", get_timestamp());
                memcpy(in_shm, &inmsg, IN_MSG_SIZE);

                // 等待客户端开启输出共享内存和输出通知信号量。
                sem_post(in_ready);
                usleep(SYNC_WAIT);
                sem_wait(in_ready);
                char out_shm_name[TIMESTAMP_LEN + 8];
                out_shm_name[TIMESTAMP_LEN + 7] = 0;
                strcpy(out_shm_name, "fs_out_");
                strncpy(out_shm_name + 7, inmsg.cmd + 8, TIMESTAMP_LEN);
                int fd = shm_open(out_shm_name, O_RDWR, 0662);
                out_shms[inmsg.uid] = mmap(NULL, OUT_BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);

                char out_sem_ready_name[TIMESTAMP_LEN + 14];
                out_sem_ready_name[TIMESTAMP_LEN + 13] = 0;
                strcpy(out_sem_ready_name, "fs_out_ready_");
                strncpy(out_sem_ready_name + 13, inmsg.cmd + 8, TIMESTAMP_LEN);
                out_readys[inmsg.uid] = sem_open(out_sem_ready_name, O_EXCL);
            }
            sem_post(in_ready);
            usleep(SYNC_WAIT);
        }
        else if (logined[inmsg.uid])
        {

            // 登出。
            if (!strncmp(inmsg.cmd, "LOGOUT", 6))
            {
                logined[inmsg.uid] = 0;
                out_shms[inmsg.uid] = NULL;
                sem_post(in_ready);
                usleep(SYNC_WAIT);
            }
            else if (inmsg.uid == 0 && !strncmp(inmsg.cmd, "shutdown", 8))
            {
                sem_post(in_ready);
                usleep(SYNC_WAIT);
                return;
            }
            else
            {
                sem_post(in_ready);
                usleep(SYNC_WAIT);
                handle_cmd(&inmsg);
            }
        }
    }
}

void handle_cmd(inmsg_t *inmsg)
{
    return;
}