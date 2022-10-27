#pragma once

#include "cmd.h"
#include "semaphore.h"

#define MAX_CMD_LINE 32

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

void *in_shm;
void *out_shm;

sem_t *in_mutex;
sem_t *in_ready;

// 开辟输入命令的共享内存。
int open_shm();

int handle_input(inmsg_t *inmsg);

int login(uint32_t uid, char *pwd);