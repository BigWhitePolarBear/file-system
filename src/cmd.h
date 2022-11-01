#pragma once

#include "stdint.h"

#define SHM_NAME "fs"
#define SEM_MUTEX_NAME "fs_mutex"
#define SEM_IN_READY_NAME "fs_in_ready"
#define SEM_OUT_READY_NAME "fs_out_ready"
#define SHM_SIZE 128
#define CMD_LEN (SHM_SIZE - 4)
#define IN_SEM_PERM (S_IRWXU | S_IRWXG | S_IRWXO)
#define MAX_USER_CNT 16
#define TIMESTAMP_LEN 13
#define SPEC_SHM_SIZE BLOCK_SIZE

// 供 fun.c 实现具体功能。
extern void *spec_shms[MAX_USER_CNT];
extern uint32_t working_dirs[MAX_USER_CNT]; // 储存目录的 ino 。

// 开辟输入命令的共享内存。
int open_shm();
// 关闭输入命令的共享内存。
int unlink_shm();

typedef struct
{
    uint32_t uid;

    char cmd[CMD_LEN];
} msg_t;

// 通过输入共享内存获取命令。
void handle_msg();
void handle_cmd(msg_t *msg);