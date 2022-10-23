#pragma once

#include "device.h"

#define BLOCK_CNT DISK_SIZE / BLOCK_SIZE
#define INODE_CNT DISK_SIZE / (8 * 1024)

// 文件系统布局：
// | 超级块 | 位图 | inodes | 数据块 |
// 由于一共有 100k 个块，因此需要 12800B 储存
// 位图，占用 13 个块。
// 每个 inode 占用 128 字节的空间，每 8KB 一个 inode，
// 因此用 15625 个块储存 125000 个inode。

typedef struct
{
    uint32_t status; // 状态: 0 正常 | 1 异常
    uint32_t fmt_time;
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
} superblock_t;

typedef struct
{
    uint8_t bytes[BLOCK_SIZE];
} bitblock_t;

typedef struct
{
    uint32_t ino;
    uint32_t size;
    uint32_t bcnt;
    uint32_t uid;
    uint32_t ctime;
    uint32_t wtime;
    uint32_t privilege; // 仅低 6 位有效，高 3 位为拥有者权限，低 3 位为其他用户权限。
    uint32_t direct_blocks[8];
    uint32_t indirect_blocks[4];
    uint32_t double_indirect_blocks[2];
    uint32_t triple_indirect_block;

    uint32_t padding[128 / 4 - 22];
} inode_t;

typedef struct
{
    uint32_t blocks[BLOCK_SIZE / 4];
} indirectblock_t;

int mkfs();
int mksb();
int mkbitmap();

int sb_update_last_wtime();

void set_bitblock(bitblock_t *bb, uint32_t pos);