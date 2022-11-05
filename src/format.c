#include "format.h"
#include "base_fun.h"
#include "bitmap.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

superblock_t sb;

int mkfs()
{
    mksb();
    printf("格式化超级块完成！ \r\n");

    if (mkbitmap())
    {
        printf("格式化位图失败！ \r\n");
        return -1;
    }
    printf("格式化位图完成！ \r\n");
    if (mkroot())
    {
        printf("创建根目录失败！ \r\n");
        return -1;
    }
    printf("格式化根目录完成！ \r\n");

    return 0;
}

void mksb()
{
    sb.status = 0;
    sb.fmt_time = (uint32_t)time(NULL);
    sb.last_wtime = (uint32_t)time(NULL);
    sb.ucnt = 1;
    sb.bsize = BLOCK_SIZE;
    sb.bcnt = BLOCK_CNT;
    sb.icnt = INODE_CNT;
    sb.free_icnt = INODE_CNT;
    sb.isize = INODE_SIZE;

    sb.inode_bcnt = (INODE_CNT * INODE_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sb.inode_bitmap_bcnt = INODE_BITMAP_BCNT;
    // 一个块中有许多位，磁盘中绝大部分块都是数据块，因此
    // data_bitmap_bcnt 即为块总数除以一个块拥有的比特数。
    sb.data_bitmap_bcnt = DATA_BITMAP_BCNT;
    sb.data_bcnt = BLOCK_CNT - 1 - sb.inode_bcnt - sb.inode_bitmap_bcnt - sb.data_bitmap_bcnt;
    sb.free_data_bcnt = BLOCK_CNT - 1 - sb.inode_bcnt - sb.inode_bitmap_bcnt - sb.data_bitmap_bcnt;

    sb.inode_table_start = 1;
    sb.inode_bitmap_start = sb.inode_table_start + sb.inode_bcnt;
    sb.data_bitmap_start = sb.inode_bitmap_start + sb.inode_bitmap_bcnt;
    sb.data_start = sb.data_bitmap_start + sb.data_bitmap_bcnt;

    sb.last_alloc_inode = 0;
    sb.last_alloc_data = 0;

    sb.users[0].uid = 0;
    strcpy(sb.users[0].pwd, "root");
    for (uint8_t i = 1; i < MAX_USER_CNT; i++)
        memset(sb.users[i].pwd, 0, PWD_LEN);

    sbwrite(false);
}

int mkroot()
{
    inode_t inode;
    inode.ino = 0;
    inode.type = 1;
    inode.ctime = (uint32_t)time(NULL);
    inode.wtime = (uint32_t)time(NULL);
    inode.uid = 0;
    inode.privilege = 077;

    // 保存自身
    inode.size = 2;
    inode.bcnt = 1;
    inode.direct_blocks[0] = sb.data_start + 0;
    dirblock_t db;
    db.direntries[0].ino = inode.ino;
    db.direntries[0].type = 1;
    db.direntries[1] = db.direntries[0];
    strcpy(db.direntries[0].name, ".");
    strcpy(db.direntries[1].name, "..");
    if (bwrite(sb.data_start, &db))
    {
        printf("写入根目录数据块失败！ \r\n");
        return -1;
    }
    if (iwrite(0, &inode))
    {
        printf("写入根目录 inode 失败！ \r\n");
        return -1;
    }

    return 0;
}

int mkbitmap()
{
    bitblock_t bb;
    memset(&bb, 0, BLOCK_SIZE);
    // 清空 bitmap 。
    for (int i = sb.inode_bitmap_start; i < sb.data_bitmap_start + DATA_BITMAP_BCNT; i++)
    {
        if (bwrite(i, &bb))
        {
            printf("写入位图失败！\n");
            return -1;
        }
        sbwrite(false);
    }
    // 已分配给根目录。
    set_inode_bitmap(0);
    set_data_bitmap(sb.data_start);
    return 0;
}