#pragma once

#include "device.h"

#define BLOCK_CNT DISK_SIZE / BLOCK_SIZE
#define INODE_CNT DISK_SIZE / (8 * 1024)

struct super_block
{
    uint32_t status; // 状态: 0 正常 | 1 异常
    uint32_t format_time;
    uint32_t last_wtime;
    uint32_t bsize;
    uint32_t bcnt;
    uint32_t free_bcnt;
    uint32_t icnt;
    uint32_t free_icnt;
    uint32_t isize;
    uint32_t data_bcnt;
    uint32_t free_data_bcnt;

    uint32_t padding[BLOCK_SIZE / 4 - 11];
};

int mkfs();

int mksb();

int sb_update_last_wtime();