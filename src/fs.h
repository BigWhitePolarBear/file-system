#pragma once

#include "device.h"

#define BLOCK_CNT DISK_SIZE / BLOCK_SIZE
#define INODE_CNT DISK_SIZE / (8 * 1024)

// 文件系统布局：
// | 超级块 | 位图 | inodes | 数据块 |
// 由于一共有 100k 个块，因此需要 12800B 储存
// 位图，占用 13 个块。

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

struct bit_block
{
    uint8_t bytes[BLOCK_SIZE];
};

struct inode
{
};

int mkfs();
int mksb();
int mkbitmap();

int sb_update_last_wtime();

void set_bit_block(struct bit_block *bb, uint32_t pos);