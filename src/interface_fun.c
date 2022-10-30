#include "interface_fun.h"
#include "assert.h"
#include "cmd.h"
#include "device.h"
#include "internal_fun.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

int login(uint32_t uid, const char pwd[])
{
    return !strncmp(sb.users[uid].pwd, pwd, PWD_LEN);
}

uint16_t info(uint32_t uid)
{
    struct tm lt;
    time_t t;

    uint16_t i = 0;
    void *spec_shm = spec_shms[uid];
    strcpy(spec_shm + i, "系统状态：");
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
    sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\r\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
            lt.tm_min, lt.tm_sec);
    i += 21;
    strcpy(spec_shm + i, "最后修改时间：");
    i += 21;
    t = sb.fmt_time;
    localtime_r(&t, &lt);
    sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\r\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
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

uint16_t ls(uint32_t uid, bool detial)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[uid];
    inode_t inode;
    if (iread(working_dirs[uid], &inode))
    {
        printf("读取 inode 失败！\n");
        strcpy(spec_shm + i, "ERROR");
        return 5;
    }
    assert(inode.type == 1);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;

    for (uint32_t j = 0; j < inode.size; j++)
    {
        if (j % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if (j < DIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if (bread(inode.direct_blocks[j % DIR_ENTRY_PER_DIRECT_BLOCK], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if (j - INDIRECT_BLOCK_OFFSET < INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(inode.indirect_blocks[(j - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - INDIRECT_BLOCK_OFFSET) % DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) < DOUBLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(inode.double_indirect_blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) %
                                                           DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) < TRIPLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
                    if (bread(inode.triple_indirect_blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                                           DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                              &triple_indiectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(triple_indiectb
                                  .blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
        }
        direntry_t de = db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK];
        if (detial)
        {
            // 权限。
            if (de.type == 1)
                strcpy(spec_shm + i++, "d");
            else
                strcpy(spec_shm + i++, "-");
            if (de.privilege & 040)
                strcpy(spec_shm + i++, "r");
            else
                strcpy(spec_shm + i++, "-");
            if (de.privilege & 020)
                strcpy(spec_shm + i++, "w");
            else
                strcpy(spec_shm + i++, "-");
            if (de.privilege & 010)
                strcpy(spec_shm + i++, "x");
            else
                strcpy(spec_shm + i++, "-");
            if (de.privilege & 004)
                strcpy(spec_shm + i++, "r");
            else
                strcpy(spec_shm + i++, "-");
            if (de.privilege & 002)
                strcpy(spec_shm + i++, "w");
            else
                strcpy(spec_shm + i++, "-");
            if (de.privilege & 001)
                strcpy(spec_shm + i++, "x");
            else
                strcpy(spec_shm + i++, "-");
            strcpy(spec_shm + i++, "\t");

            // 拥有者。
            sprintf(spec_shm + i, "%2u\t", de.uid);
            i += 3;

            // 文件大小或目录元数据占用空间。
            if (de.type == 0)
            {
                sprintf(spec_shm + i, "%u\t", de.size);
                i += num2width(de.size) + 1;
            }
            else
            {
                sprintf(spec_shm + i, "%u\t", de.bcnt * BLOCK_SIZE);
                i += num2width(de.bcnt * BLOCK_SIZE) + 1;
            }

            struct tm lt;
            time_t t;
            // 创建时间。
            t = de.ctime;
            localtime_r(&t, &lt);
            sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\t", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                    lt.tm_hour, lt.tm_min, lt.tm_sec);
            i += 20;

            // 创建时间。
            t = de.wtime;
            localtime_r(&t, &lt);
            sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\t", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                    lt.tm_hour, lt.tm_min, lt.tm_sec);
            i += 20;

            // 名称
            strcpy(spec_shm + i, de.name);
            i += strlen(de.name);
            if (de.type == 1)
                strcpy(spec_shm + i++, "/");

            if (j + 1 < inode.size)
            {
                strcpy(spec_shm + i, "\r\n");
                i += 2;
            }
        }
        else
        {
            strcpy(spec_shm + i, de.name);
            i += strlen(de.name);
            if (de.type == 1)
                strcpy(spec_shm + i++, "/");
            strcpy(spec_shm + i++, "\t");

            if (j > 0 && j % 5 == 0)
            {
                strcpy(spec_shm + i, "\r\n");
                i += 2;
            }
        }
    }
    strcpy(spec_shm + i, "\r\n");
    i += 2;

    return i;
}

uint16_t unknown(uint32_t uid)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[uid];
    strcpy(spec_shm + i, "无法识别该命令！\r\n");
    i += 26;
    return 0;
}
