#pragma once

#include "common.h"
#include "device.h"

// 文件系统布局：
// | 超级块 | inode Table | inode 位图 | 数据块位图 | 数据块 |
// 各种参数可以到 common.h 中修改。

typedef struct
{
    uint32_t status; // 状态: 0 正常 | 1 异常
    uint32_t fmt_time;
    uint32_t last_wtime;
    uint32_t bsize;
    uint32_t bcnt;
    uint32_t icnt;
    uint32_t free_icnt;
    uint32_t isize;

    uint32_t inode_bcnt;
    uint32_t data_bcnt;
    uint32_t free_data_bcnt;
    uint32_t inode_bitmap_bcnt;
    uint32_t data_bitmap_bcnt;

    // 各部分的起始块号
    uint32_t inode_table_start;
    uint32_t inode_bitmap_start;
    uint32_t data_bitmap_start;
    uint32_t data_start;

    // 存储上次分配的 inode 或数据块的编号，下次分配从其后开始查找。
    uint32_t last_alloc_inode;
    uint32_t last_alloc_data;

    uint32_t padding[BLOCK_SIZE / 4 - 19];
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

    uint32_t padding[INODE_SIZE / 4 - 22];
} inode_t;

typedef struct
{
    uint32_t blocks[BLOCK_SIZE / 4];
} indirectblock_t;

extern superblock_t sb;

int mkfs();
int mksb();
int mkbitmap();
int mkroot();
int mkdir();

int sb_update_last_wtime();

// 返回 0xffffffff 即为异常。
uint32_t get_free_inode();
uint32_t get_free_data();

int set_inode_bitmap(uint32_t pos);
int set_data_bitmap(uint32_t pos);

uint8_t test_bitblock(bitblock_t *bb, uint32_t pos);
void set_bitblock(bitblock_t *bb, uint32_t pos);