#include "fun.h"
#include "assert.h"
#include "bitmap.h"
#include "cmd.h"
#include "device.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

uint32_t get_free_inode()
{
    if (sb.free_icnt == 0)
    {
        printf("空闲 inode 数量不足，无法分配！\n");
        return 0xffffffff;
    }

    bitblock_t bb;
    uint32_t ino;
    if (sb.last_alloc_inode < sb.icnt - 1)
        ino = sb.last_alloc_inode + 1;
    else
        ino = 1; // 从头分配，编号为 0 的 inode 为根目录。

    while (1)
    {
        if (bread(sb.inode_bitmap_start + ino / BIT_PER_BLOCK, &bb))
        {
            printf("读取位图失败！\n");
            return 0xffffffff;
        }
        while (ino % BIT_PER_BLOCK > 0)
        {
            if (test_bitblock(&bb, ino % BIT_PER_BLOCK))
            {
                set_bitblock(&bb, ino % BIT_PER_BLOCK);
                if (bwrite(sb.inode_bitmap_start + ino / BIT_PER_BLOCK, &bb))
                {
                    printf("写入位图失败！\n");
                    return 0xffffffff;
                }
                sb.free_icnt--;
                sb.last_alloc_inode = ino;
                sbwrite();
                return ino;
            }
            if (ino < sb.icnt - 1)
                ino++;
            else
                ino = 1; // 从头分配，编号为 0 的 inode 为根目录。

            // 避免死循环，如果以下 if 分支被执行，说明文件系统出现了不一致。
            if (ino == sb.last_alloc_inode)
            {
                printf("未找到空闲 inode ，无法分配！\n");
                return 0xffffffff;
            }
        }
    }
}

uint32_t get_free_data()
{
    if (sb.free_data_bcnt == 0)
    {
        printf("空闲数据块数量不足，无法分配！\n");
        return 0xffffffff;
    }

    bitblock_t bb;
    uint32_t bno;
    if (sb.last_alloc_data < sb.data_bcnt - 1)
        bno = sb.last_alloc_data + 1;
    else
        bno = 1; // 从头分配，编号为 0 的数据块分配给了根目录。

    while (1)
    {
        if (bread(sb.data_bitmap_start + bno / BIT_PER_BLOCK, &bb))
        {
            printf("读取位图失败！\n");
            return 0xffffffff;
        }
        while (bno % BIT_PER_BLOCK > 0)
        {
            if (test_bitblock(&bb, bno % BIT_PER_BLOCK))
            {
                set_bitblock(&bb, bno % BIT_PER_BLOCK);
                if (bwrite(sb.inode_bitmap_start + bno / BIT_PER_BLOCK, &bb))
                {
                    printf("写入位图失败！\n");
                    return 0xffffffff;
                }
                sb.free_data_bcnt--;
                sb.last_alloc_data = bno;
                sbwrite();
                return bno;
            }
            if (bno < sb.data_bcnt - 1)
                bno++;
            else
                bno = 0;

            // 避免死循环，如果以下 if 分支被执行，说明文件系统出现了不一致。
            if (bno == sb.last_alloc_data)
            {
                printf("未找到空闲数据块，无法分配！\n");
                return 0xffffffff;
            }
        }
    }
}

int iread(uint32_t ino, inode_t *const inode)
{
    assert(ino < sb.icnt);

    itableblock_t itb;
    if (bread(sb.inode_table_start + ino / INODE_PER_BLOCK, &itb))
    {
        printf("读取 inode 对应的数据块失败！\n");
        return -1;
    }
    *inode = itb.inodes[ino % INODE_PER_BLOCK];

    return 0;
}

int iwrite(uint32_t ino, const inode_t *const inode)
{
    assert(ino < sb.icnt);

    itableblock_t itb;
    if (bread(sb.inode_table_start + ino / INODE_PER_BLOCK, &itb))
    {
        printf("读取 inode 对应的数据块失败！\n");
        return -1;
    }
    itb.inodes[ino % INODE_PER_BLOCK] = *inode;
    if (bwrite(sb.inode_table_start + ino / INODE_PER_BLOCK, &itb))
    {
        printf("写入 inode 对应的数据块失败！\n");
        return -1;
    }
    sbwrite();

    return 0;
}

void sbwrite()
{
    sb.last_wtime = (uint32_t)time(NULL);
    if (bwrite(0, &sb))
    {
        printf("写入超级块失败！\n");
        printf("致命错误，退出系统！\n");
        exit(-1);
    }
}

void sbinit()
{
    if (bread(0, &sb))
    {
        printf("读取超级块失败！\n");
        printf("致命错误，退出系统！\n");
        exit(-1);
    }
}

int login(uint32_t uid, const char pwd[])
{
    return !strncmp(sb.users[uid].pwd, pwd, PWD_LEN);
}

uint16_t info(void *const spec_shm)
{
    struct tm lt;
    time_t t;

    uint16_t i = 0;
    strcpy(spec_shm, "系统状态：");
    i += 15;
    if (sb.status)
        strcpy(spec_shm + i, "异常\r\n");
    else
        strcpy(spec_shm + i, "正常\r\n");
    i += 8;
    strcpy(spec_shm + i, "格式化时间：");
    i += 18;
    t = sb.fmt_time;
    localtime_r(&t, &lt);
    sprintf(spec_shm + i, "%4d-%2d-%2d %2d:%2d:%2d\r\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
            lt.tm_min, lt.tm_sec);
    i += 21;
    strcpy(spec_shm + i, "最后修改时间：");
    i += 21;
    t = sb.fmt_time;
    localtime_r(&t, &lt);
    sprintf(spec_shm + i, "%4d-%2d-%2d %2d:%2d:%2d\r\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
            lt.tm_min, lt.tm_sec);
    i += 21;
    strcpy(spec_shm + i, "用户数量：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.ucnt);
    i += 2 + num2width(sb.ucnt);
    strcpy(spec_shm + i, "块大小：");
    i += 12;
    sprintf(spec_shm + i, "%u\r\n", sb.bsize);
    i += 2 + num2width(sb.bsize);
    strcpy(spec_shm + i, "块数量：");
    i += 12;
    sprintf(spec_shm + i, "%u\r\n", sb.bcnt);
    i += 2 + num2width(sb.bcnt);
    strcpy(spec_shm + i, "inode 数量：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.icnt);
    i += 2 + num2width(sb.icnt);
    strcpy(spec_shm + i, "空闲 inode 数量：");
    i += 22;
    sprintf(spec_shm + i, "%u\r\n", sb.free_icnt);
    i += 2 + num2width(sb.free_icnt);
    strcpy(spec_shm + i, "inode 大小：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.isize);
    i += 2 + num2width(sb.isize);
    strcpy(spec_shm + i, "inode 块数量：");
    i += 18;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bcnt);
    i += 2 + num2width(sb.inode_bcnt);
    strcpy(spec_shm + i, "数据块数量：");
    i += 18;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bcnt);
    i += 2 + num2width(sb.data_bcnt);
    strcpy(spec_shm + i, "空闲数据块数量：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.free_data_bcnt);
    i += 2 + num2width(sb.free_data_bcnt);
    strcpy(spec_shm + i, "inode 位图块数量：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bitmap_bcnt);
    i += 2 + num2width(sb.inode_bitmap_bcnt);
    strcpy(spec_shm + i, "数据块位图块数量：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bitmap_bcnt);
    i += 2 + num2width(sb.data_bitmap_bcnt);
    strcpy(spec_shm + i, "inode table 起始块号：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_table_start);
    i += 2 + num2width(sb.inode_table_start);
    strcpy(spec_shm + i, "inode 位图起始块号：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bitmap_start);
    i += 2 + num2width(sb.inode_bitmap_start);
    strcpy(spec_shm + i, "数据块位图起始块号：");
    i += 30;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bitmap_start);
    i += 2 + num2width(sb.data_bitmap_start);
    strcpy(spec_shm + i, "数据块起始块号：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.data_start);
    i += 2 + num2width(sb.data_start);

    return i;
}

uint16_t num2width(uint32_t num)
{
    uint16_t width = 0;
    while (num > 0)
    {
        width++;
        num /= 10;
    }
    return width;
}