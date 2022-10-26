#pragma once

#include "stdint.h"

#define IN_SHM_NAME "fs_in"
#define IN_SEM_MUTEX_NAME "fs_in_mutex"
#define IN_SEM_READY_NAME "fs_in_ready"
#define IN_MSG_SIZE 128
#define IN_SEM_PERM (S_IRWXU | S_IRWXG | S_IRWXO) // 所有用户进程都可以访问输入相关的信号量。
#define SYNC_WAIT 5

// 开辟输入命令的共享内存。
int open_shm();
// 关闭输入命令的共享内存。
int unlink_shm();

typedef struct
{
    uint32_t uid;

    char cmd[124];
} inmsg_t;

// 通过共享内存获取命令。
void handle_msg();