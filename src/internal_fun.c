#include "internal_fun.h"
#include "assert.h"
#include "bitmap.h"
#include "device.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "unistd.h"

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
    uint32_t bno; // 临时名称，返回时会加上数据块起始偏移量。
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
                return sb.data_start + bno;
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

uint32_t search_file(uint32_t dir_ino, char filename[], uint32_t *type)
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return 0xfffffffe;
    }
    assert(dir_inode.type == 1);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;

    for (uint32_t j = 0; j < dir_inode.size; j++)
    {
        if (j % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if (j < DIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if (bread(dir_inode.direct_blocks[j / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffe;
                }
            }
            else if (j - INDIRECT_BLOCK_OFFSET < INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.indirect_blocks[(j - INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffe;
                    }
                if (bread(indirectb.blocks[(j - INDIRECT_BLOCK_OFFSET) % DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffe;
                }
            }
            else if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) < DOUBLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.double_indirect_blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) /
                                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffe;
                    }
                if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffe;
                    }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) / DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffe;
                }
            }
            else if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) < TRIPLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.triple_indirect_blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) /
                                                               DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                              &triple_indiectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffe;
                    }
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(triple_indiectb
                                  .blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffe;
                    }
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffe;
                    }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffe;
                }
            }
        }
        if (!strcmp(db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name, filename))
        {
            if (type != NULL)
                *type = db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].type;
            return db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].ino;
        }
    }
    return 0xffffffff;
}

int create_file(uint32_t dir_ino, uint32_t uid, uint32_t type, char filename[])
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return -2;
    }
    assert(dir_inode.type == 1);
    if (dir_inode.size == DIRECT_BLOCK_DIR_ENTRY_CNT + INDIRECT_BLOCK_DIR_ENTRY_CNT +
                              DOUBLE_INDIRECT_BLOCK_DIR_ENTRY_CNT + TRIPLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
        return -1;

    uint32_t db_bno;
    dirblock_t db;
    if (dir_inode.size + 1 < DIRECT_BLOCK_DIR_ENTRY_CNT)
    {
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((dir_inode.direct_blocks[dir_inode.size / DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            dir_inode.bcnt++;
        }
        db_bno = dir_inode.direct_blocks[dir_inode.size / DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -2;
        }
    }
    else if (dir_inode.size + 1 - INDIRECT_BLOCK_OFFSET < INDIRECT_BLOCK_DIR_ENTRY_CNT)
    {
        indirectblock_t indirectb;
        if ((dir_inode.size - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if ((dir_inode.indirect_blocks[(dir_inode.size - INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
        }
        if (bread(dir_inode.indirect_blocks[(dir_inode.size - INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -2;
        }
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((indirectb.blocks[(dir_inode.size - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(
                    dir_inode.indirect_blocks[(dir_inode.size - INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                    &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -2;
            }
        }
        db_bno = indirectb.blocks[(dir_inode.size - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -2;
        }
    }
    else if (dir_inode.size + 1 - DOUBLE_INDIRECT_BLOCK_OFFSET < DOUBLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
    {
        indirectblock_t double_indirectb;
        if ((dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if ((dir_inode.double_indirect_blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) /
                                                  DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
        }
        if (bread(dir_inode.double_indirect_blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) /
                                                   DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级中间目录块失败！\n");
            return -2;
        }
        indirectblock_t indirectb;
        if ((dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if ((double_indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) %
                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(dir_inode.double_indirect_blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) /
                                                        DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                       &double_indirectb))
            {
                printf("写入二级中间目录块失败！\n");
                return -2;
            }
        }
        if (bread(double_indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -2;
        }
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -2;
            }
        }
        db_bno = indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -2;
        }
    }
    else
    {
        indirectblock_t triple_indirectb;
        if ((dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
        {
            if ((dir_inode.triple_indirect_blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) /
                                                  DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
        }
        if (bread(dir_inode.triple_indirect_blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) /
                                                   DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indirectb))
        {
            printf("读取三级中间目录块失败！\n");
            return -2;
        }
        indirectblock_t double_indirectb;
        if ((dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if ((triple_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(dir_inode.triple_indirect_blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) /
                                                        DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                       &triple_indirectb))
            {
                printf("写入三级中间目录块失败！\n");
                return -2;
            }
        }
        if (bread(triple_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                          DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级中间目录块失败！\n");
            return -2;
        }
        indirectblock_t indirectb;
        if ((dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if ((double_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(
                    triple_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                            DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                    &double_indirectb))
            {
                printf("写入二级中间目录块失败！\n");
                return -2;
            }
        }
        if (bread(double_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -2;
        }
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(double_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -2;
            }
        }
        db_bno = indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -2;
        }
    }
    direntry_t *de = &db.direntries[dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK];
    de->type = type;
    strcpy(de->name, filename);
    if ((de->ino = get_free_inode()) == 0xffffffff)
    {
        printf("获取空闲 inode 失败！\n");
        return -2;
    }
    inode_t inode;
    if (iread(de->ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -2;
    }
    inode.ino = de->ino;
    inode.type = type;
    if (inode.type == 1)
    {
        inode.size = 2;
        inode.bcnt = 1;
    }
    inode.uid = uid;
    inode.ctime = (uint32_t)time(NULL);
    inode.wtime = (uint32_t)time(NULL);
    inode.privilege = 066;
    if ((inode.direct_blocks[0] = get_free_data()) == 0xffffffff)
    {
        printf("获取空闲数据块失败！\n");
        return -2;
    }
    dirblock_t new_db;
    if (bread(inode.direct_blocks[0], &new_db))
    {
        printf("读取目录块失败！\n");
        return -2;
    }
    new_db.direntries[0].ino = inode.ino;
    new_db.direntries[0].type = inode.type;
    new_db.direntries[1] = new_db.direntries[0];
    strcpy(new_db.direntries[0].name, ".");
    strcpy(new_db.direntries[1].name, "..");
    if (bwrite(inode.direct_blocks[0], &new_db))
    {
        printf("写入目录块失败！\n");
        return -2;
    }
    if (bwrite(db_bno, &db))
    {
        printf("写入目录块失败！\n");
        return -2;
    }
    if (iwrite(de->ino, &inode))
    {
        printf("写入 inode 失败！\n");
        return -2;
    }

    dir_inode.size++;
    if (iwrite(dir_ino, &dir_inode))
    {
        printf("写入 inode 失败！\n");
        return -2;
    }

    return 0;
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