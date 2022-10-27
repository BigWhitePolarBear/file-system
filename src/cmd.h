#pragma once

#include "stdint.h"

#define IN_SHM_NAME "fs_in"
#define IN_SEM_MUTEX_NAME "fs_in_mutex"
#define IN_SEM_READY_NAME "fs_in_ready"
#define IN_MSG_SIZE 128
#define CMD_LEN IN_MSG_SIZE - 4
#define IN_SEM_PERM (S_IRWXU | S_IRWXG | S_IRWXO) // 所有用户进程都可以访问输入相关的信号量。
#define SYNC_WAIT 5
#define MAX_USER_CNT 16
#define TIMESTAMP_LEN 13
#define OUT_BUF_SIZE 512

// 开辟输入命令的共享内存。
int open_shm();
// 关闭输入命令的共享内存。
int unlink_shm();

typedef struct
{
    uint32_t uid;

    char cmd[CMD_LEN];
} inmsg_t;

// 通过输入共享内存获取命令。
void handle_msg();
void handle_cmd(char cmd[], void *out_shm);