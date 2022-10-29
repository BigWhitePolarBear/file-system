#pragma once

#include "cmd.h"
#include "semaphore.h"

#define MAX_CMD_LINE 32
#define OUT_SEM_PERM (S_IRWXU | S_IRWXG | S_IRWXO)

uint8_t cmdline_cnt;

struct inbuf_t
{
    struct inbuf_t *pre;
    struct inbuf_t *nxt;
    uint8_t len;
    char cmd[CMD_LEN];
};

typedef struct inbuf_t inbuf_t;

inbuf_t *head, *tail, *cur;

void *shm;
void *spec_shm;

sem_t *mutex;
sem_t *in_ready;
sem_t *out_ready;
sem_t *spec_out_ready;

// 开辟输入命令的共享内存。
int open_shm();

// 将 shell 输入保存到 msg 中，并返回输入长度。
// 若返回 -1 ，说明当前 shell 登录了 shell 账号并输入了 shutdown ，
// 此时 shell 应关闭专属共享内存和信号量并退出。
int handle_input(msg_t *msg);