#pragma once

#include "stdint.h"

// 文件系统布局：
// | 超级块 | inode Table | inode 位图 | 数据块位图 | 数据块 |

#define DISK_SIZE (100 * 1024 * 1024)
#define BLOCK_SIZE 1024
#define BIT_PER_BLOCK (8 * BLOCK_SIZE)
#define BLOCK_CNT (DISK_SIZE / BLOCK_SIZE)
#define STORE_SIZE_PER_INODE (4 * BLOCK_SIZE)
#define INODE_CNT (DISK_SIZE / STORE_SIZE_PER_INODE)
#define INODE_SIZE 128
#define INODE_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define DIR_ENTRY_SIZE 64
#define MAX_USER_CNT 16
#define PWD_LEN 16

#define DIRECT_BLOCK_CNT 8
#define INDIRECT_BLOCK_CNT 4
#define DOUBLE_INDIRECT_BLOCK_CNT 2
#define TRIPLE_INDIRECT_BLOCK_CNT 1
#define BNO_PER_BLOCK (BLOCK_SIZE / 4)

#define DIR_ENTRY_PER_DIRECT_BLOCK (BLOCK_SIZE / DIR_ENTRY_SIZE)
#define DIR_ENTRY_PER_INDIRECT_BLOCK (BNO_PER_BLOCK * DIR_ENTRY_PER_DIRECT_BLOCK)
#define DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK (BNO_PER_BLOCK * DIR_ENTRY_PER_INDIRECT_BLOCK)
#define DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK (BNO_PER_BLOCK * DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK)

#define DIRECT_DIR_ENTRY_CNT (DIRECT_BLOCK_CNT * DIR_ENTRY_PER_DIRECT_BLOCK)
#define INDIRECT_DIR_OFFSET DIRECT_DIR_ENTRY_CNT
#define INDIRECT_DIR_ENTRY_CNT (INDIRECT_BLOCK_CNT * DIR_ENTRY_PER_INDIRECT_BLOCK)
#define DOUBLE_INDIRECT_DIR_OFFSET (INDIRECT_DIR_OFFSET + INDIRECT_DIR_ENTRY_CNT)
#define DOUBLE_INDIRECT_DIR_ENTRY_CNT (DOUBLE_INDIRECT_BLOCK_CNT * DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK)
#define TRIPLE_INDIRECT_DIR_OFFSET (DOUBLE_INDIRECT_DIR_OFFSET + DOUBLE_INDIRECT_DIR_ENTRY_CNT)
#define TRIPLE_INDIRECT_DIR_ENTRY_CNT (TRIPLE_INDIRECT_BLOCK_CNT * DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK)

#define DATA_BLOCK_PER_INDIRECT_BLOCK BNO_PER_BLOCK
#define DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK (BNO_PER_BLOCK * DATA_BLOCK_PER_INDIRECT_BLOCK)
#define DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK (BNO_PER_BLOCK * DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK)

#define DIRECT_DATA_BLOCK_CNT DIRECT_BLOCK_CNT
#define INDIRECT_DATA_BLOCK_OFFSET DIRECT_DATA_BLOCK_CNT
#define INDIRECT_DATA_BLOCK_CNT (INDIRECT_BLOCK_CNT * DATA_BLOCK_PER_INDIRECT_BLOCK)
#define DOUBLE_INDIRECT_DATA_BLOCK_OFFSET (INDIRECT_DATA_BLOCK_OFFSET + INDIRECT_DATA_BLOCK_CNT)
#define DOUBLE_INDIRECT_DATA_BLOCK_CNT (DOUBLE_INDIRECT_BLOCK_CNT * DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK)
#define TRIPLE_INDIRECT_DATA_BLOCK_OFFSET (DOUBLE_INDIRECT_DATA_BLOCK_OFFSET + DOUBLE_INDIRECT_DATA_BLOCK_CNT)
#define TRIPLE_INDIRECT_BDATA_BLOCK_CNT (TRIPLE_INDIRECT_BLOCK_CNT * DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK)

typedef struct
{
    uint32_t uid;

    char pwd[PWD_LEN];
} user_t;

typedef struct
{
    uint32_t status; // 状态: 0 正常 | 1 异常
    uint32_t fmt_time;
    uint32_t last_wtime;
    uint32_t ucnt;
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

    user_t users[MAX_USER_CNT];

    uint32_t padding[BLOCK_SIZE / 4 - 100];
} superblock_t;

typedef struct
{
    uint8_t bytes[BLOCK_SIZE];
} bitblock_t;

typedef struct
{
    uint32_t ino;
    uint32_t type; // 为 0 时为文件，为 1 时为目录。
    uint32_t size; // 当 inode 储存目录时， size 代表目录项数量。
    uint32_t bcnt;
    uint32_t uid;
    uint32_t ctime;
    uint32_t wtime;
    uint32_t privilege; // 仅低 8 位有效，高 4 位中的低 3 位为拥有者权限，低 4 位中的低 3 位为其他用户权限。
    uint32_t direct_blocks[DIRECT_BLOCK_CNT];
    uint32_t indirect_blocks[INDIRECT_BLOCK_CNT];
    uint32_t double_indirect_blocks[DOUBLE_INDIRECT_BLOCK_CNT];
    uint32_t triple_indirect_blocks[TRIPLE_INDIRECT_BLOCK_CNT];

    uint32_t padding[INODE_SIZE / 4 - 23];
} inode_t;

typedef struct
{
    uint32_t blocks[BNO_PER_BLOCK];
} indirectblock_t;

typedef struct
{
    inode_t inodes[INODE_PER_BLOCK];
} itableblock_t;

#define FILE_NAME_LEN (DIR_ENTRY_SIZE - 4 * 3)
typedef struct
{
    uint32_t ino;
    uint32_t type;

    char name[FILE_NAME_LEN];
} direntry_t;

typedef struct
{
    direntry_t direntries[DIR_ENTRY_PER_DIRECT_BLOCK];
} dirblock_t;

typedef struct
{
    uint8_t data[BLOCK_SIZE];
} datablock_t;

extern superblock_t sb;

uint64_t get_timestamp();

uint16_t uint2width(uint32_t num);

// 只计算整数部分。
uint16_t float2width(float num);