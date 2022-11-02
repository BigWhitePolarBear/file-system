#include "cmd.h"
#include "common.h"
#include "fcntl.h"
#include "interface_fun.h"
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

uint8_t logined[MAX_USER_CNT * MAX_SESSION_PER_USER];
void *spec_shms[MAX_USER_CNT * MAX_SESSION_PER_USER];
uint16_t last_spec_shm_pos[MAX_USER_CNT * MAX_SESSION_PER_USER];
sem_t *spec_out_readys[MAX_USER_CNT * MAX_SESSION_PER_USER];
uint32_t working_dirs[MAX_USER_CNT * MAX_SESSION_PER_USER];

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

uint32_t session_id2uid(uint32_t session_id)
{
    return session_id / MAX_SESSION_PER_USER;
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

        if (!strncmp(msg.cmd, "LOGIN", 5))
        {
            // 检查当前账号会话是否到达上限。
            for (uint8_t i = 0; i < MAX_SESSION_PER_USER; i++)
                if (!logined[msg.session_id * 4 + i])
                {
                    msg.session_id = msg.session_id * 4 + i;
                    break;
                }

            // 登录信息。
            char pwd[PWD_LEN];
            strncpy(pwd, msg.cmd + 6, PWD_LEN);
            if (login(session_id2uid(msg.session_id), pwd)) // 密码正确
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
                spec_shms[msg.session_id] = mmap(NULL, SPEC_SHM_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
                if (*(int *)spec_shms[msg.session_id] == -1)
                {
                    printf("映射专属共享内存失败！\n");
                    sem_post(out_ready);
                    continue;
                }
                memset(spec_shms[msg.session_id], 0, SPEC_SHM_SIZE);

                char spec_sem_out_ready_name[TIMESTAMP_LEN + 14];
                spec_sem_out_ready_name[TIMESTAMP_LEN + 13] = 0;
                strcpy(spec_sem_out_ready_name, "fs_out_ready_");
                strncpy(spec_sem_out_ready_name + 13, msg.cmd + 8, TIMESTAMP_LEN);
                spec_out_readys[msg.session_id] = sem_open(spec_sem_out_ready_name, O_EXCL);
                if (spec_out_readys[msg.session_id] == SEM_FAILED)
                {
                    spec_shms[msg.session_id] = NULL;
                    printf("连接专属输出通知信号量失败！\n");
                    sem_post(out_ready);
                    continue;
                }

                // 再次通知 shell
                strcpy(msg.cmd, "SUCCESS AGAIN");
                memcpy(shm, &msg, SHM_SIZE);
                logined[msg.session_id] = 1;
                working_dirs[msg.session_id] = 0;
            }
            sem_post(out_ready);
        }
        else if (logined[msg.session_id])
        {
            // 登出。
            if (!strncmp(msg.cmd, "LOGOUT", 6))
            {
                logined[msg.session_id] = 0;
                working_dirs[msg.session_id] = 0xffffffff;
                spec_shms[msg.session_id] = NULL;
                sem_post(out_ready);
            }
            else if (msg.session_id == 0 && (!strncmp(msg.cmd, "shutdown", 8)))
            {
                memset(spec_shms[msg.session_id], 0, last_spec_shm_pos[msg.session_id]);
                sem_post(out_ready);
                sem_post(spec_out_readys[msg.session_id]);
                return;
            }
            else
            {
                sem_post(out_ready);
                handle_cmd(&msg);
                sem_post(spec_out_readys[msg.session_id]);
            }
        }
    }
}

void handle_cmd(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];
    memset(spec_shm, 0, last_spec_shm_pos[msg->session_id]);

    if (!strncmp(msg->cmd, "info", 4))
        last_spec_shm_pos[msg->session_id] = info(msg->session_id);
    else if (!strncmp(msg->cmd, "cd", 2))
        last_spec_shm_pos[msg->session_id] = cd(msg);
    else if (!strncmp(msg->cmd, "ls", 2) || !strncmp(msg->cmd, "dir", 3))
        last_spec_shm_pos[msg->session_id] = ls(msg);
    else if (!strncmp(msg->cmd, "mkdir", 5) || !strncmp(msg->cmd, "md", 2))
        last_spec_shm_pos[msg->session_id] = md(msg);
    else if (!strncmp(msg->cmd, "rd", 2) || !strncmp(msg->cmd, "rmdir", 5))
        last_spec_shm_pos[msg->session_id] = rd(msg);
    else if (!strncmp(msg->cmd, "newfile", 7))
        last_spec_shm_pos[msg->session_id] = newfile(msg);
    else if (!strncmp(msg->cmd, "rm", 2) || !strncmp(msg->cmd, "del", 3))
        last_spec_shm_pos[msg->session_id] = rm(msg);
    else if (!strncmp(msg->cmd, "cat", 3))
        last_spec_shm_pos[msg->session_id] = cat(msg);
    else
        last_spec_shm_pos[msg->session_id] = unknown(msg->session_id);

    return;
}