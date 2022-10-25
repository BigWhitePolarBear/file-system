#include "fs.h"
#include "stdlib.h"
#include "time.h"

superblock_t sb;

int mkfs()
{
    mksb();

    if (mkbitmap())
    {
        printf("格式化位图失败！\n");
        return -1;
    }

    if (mkroot())
    {
        printf("创建根目录失败！\n");
        return -1;
    }

    return 0;
}

void mksb()
{
    sb.status = 0;
    sb.fmt_time = (uint32_t)time(NULL);
    sb.last_wtime = (uint32_t)time(NULL);
    sb.bsize = BLOCK_SIZE;
    sb.bcnt = BLOCK_CNT;
    sb.icnt = INODE_CNT;
    sb.free_icnt = INODE_CNT;
    sb.isize = INODE_SIZE;

    sb.inode_bcnt = (INODE_CNT * INODE_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sb.inode_bitmap_bcnt = (INODE_CNT + BIT_PER_BLOCK - 1) / BIT_PER_BLOCK;
    // 一个块中有许多位，磁盘中绝大部分块都是数据块，因此
    // data_bitmap_bcnt 即为块总数除以一个块拥有的比特数。
    sb.data_bitmap_bcnt = (BLOCK_CNT + BIT_PER_BLOCK - 1) / BIT_PER_BLOCK;
    sb.data_bcnt = BLOCK_CNT - 1 - sb.inode_bcnt - sb.inode_bitmap_bcnt - sb.data_bitmap_bcnt;
    sb.free_data_bcnt = BLOCK_CNT - 1 - sb.inode_bcnt - sb.inode_bitmap_bcnt - sb.data_bitmap_bcnt;

    sb.inode_table_start = 1;
    sb.inode_bitmap_start = sb.inode_table_start + sb.inode_bcnt;
    sb.data_bitmap_start = sb.inode_bitmap_start + sb.inode_bitmap_bcnt;
    sb.data_start = sb.data_bitmap_start + sb.data_bitmap_bcnt;

    sb_write();
}

int mkroot()
{
    inode_t inode;
    inode.ino = 0;
    inode.ctime = (uint32_t)time(NULL);
    inode.uid = 0;
    inode.privilege = 0x00000077;

    if (iwrite(0, &inode))
    {
        printf("写入根目录 inode 失败！\n");
        return -1;
    }
    sb_update_last_wtime();
    return 0;
}

int mkdir(const char *const name)
{
    uint32_t ino = get_free_inode();
    if (ino == 0xffffffff)
    {
        printf("获取空闲 inode 失败！\n");
        return -1;
    }
    return 0;
}

int mkbitmap()
{
    bitblock_t bb;
    // 清空 bitmap 。
    for (int i = sb.inode_bitmap_start; i < sb.data_bitmap_start + sb.data_bitmap_bcnt; i++)
    {
        if (bwrite(i, &bb))
        {
            printf("持久化 bitmap 失败！\n");
            return -1;
        }
        sb_update_last_wtime();
    }
    return 0;
}

void sb_write()
{
    if (bwrite(0, &sb))
    {
        printf("持久化超级块失败！\n");
        printf("致命错误，退出系统！\n");
        exit(-1);
    }
}

void sb_update_last_wtime()
{
    sb.last_wtime = (uint32_t)time(NULL);
    if (bwrite(0, &sb))
    {
        printf("持久化超级块失败！\n");
        printf("致命错误，退出系统！\n");
        exit(-1);
    }
}
