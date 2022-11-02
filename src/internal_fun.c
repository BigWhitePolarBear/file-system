#include "internal_fun.h"
#include "assert.h"
#include "bitmap.h"
#include "device.h"
#include "free.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "unistd.h"

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

uint32_t search(uint32_t dir_ino, const char filename[], uint32_t *type, bool delete, bool force)
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return 0xfffffffd;
    }
    assert(dir_inode.type == 1);
    assert(dir_inode.size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                 TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;
    uint32_t dbno;

    for (uint32_t j = 0; j < dir_inode.size; j++)
    {
        if (j % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if (j < DIRECT_DIR_ENTRY_CNT)
            {
                if (bread(dir_inode.direct_blocks[j / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffd;
                }
                dbno = dir_inode.direct_blocks[j / DIR_ENTRY_PER_DIRECT_BLOCK];
            }
            else if (j - INDIRECT_DIR_OFFSET < INDIRECT_DIR_ENTRY_CNT)
            {
                if ((j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                {
                    if (bread(dir_inode.indirect_blocks[(j - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffd;
                    }
                }
                if (bread(indirectb.blocks[(j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIR_ENTRY_PER_DIRECT_BLOCK],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffd;
                }
                dbno =
                    indirectb
                        .blocks[(j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIR_ENTRY_PER_DIRECT_BLOCK];
            }
            else if ((j - DOUBLE_INDIRECT_DIR_OFFSET) < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
            {
                if ((j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                {
                    if (bread(dir_inode.double_indirect_blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取二级目录中间块失败！\n");
                        return 0xfffffffd;
                    }
                }
                if ((j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                {
                    if (bread(
                            double_indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) %
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffd;
                    }
                }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIR_ENTRY_PER_DIRECT_BLOCK],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffd;
                }
                dbno = indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DIRECT_BLOCK];
            }
            else if ((j - TRIPLE_INDIRECT_DIR_OFFSET) < TRIPLE_INDIRECT_DIR_ENTRY_CNT)
            {
                if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
                {
                    if (bread(dir_inode.triple_indirect_blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                              &triple_indiectb))
                    {
                        printf("读取三级目录中间块失败！\n");
                        return 0xfffffffd;
                    }
                }
                if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                {
                    if (bread(triple_indiectb
                                  .blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK /
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取二级目录中间块失败！\n");
                        return 0xfffffffd;
                    }
                }
                if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                {
                    if (bread(
                            double_indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) %
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return 0xfffffffd;
                    }
                }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIR_ENTRY_PER_DIRECT_BLOCK],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    return 0xfffffffd;
                }
                dbno = indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DIRECT_BLOCK];
            }
        }
        if (!strcmp(db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name, filename))
        {
            if (type != NULL)
                *type = db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].type;
            uint32_t ino = db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].ino;
            if (delete)
            {
                inode_t inode;
                if (iread(ino, &inode))
                {
                    printf("读取 inode 失败！\n");
                    return 0xfffffffd;
                }
                if (!force && inode.size > 2)
                    return 0xfffffffe;

                direntry_t de = get_last_direntry(&dir_inode);
                if (de.ino == 0)
                {
                    printf("获取最后一个目录项失败！\n");
                    return 0xfffffffd;
                }
                if (strcmp(filename, de.name))
                {
                    // 将最后一个目录项复制到当前位置。
                    db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK] = de;
                    if (bwrite(dbno, &db))
                    {
                        printf("修改数据块位图失败！\n");
                        return 0xfffffffd;
                    }
                }

                dir_inode.size--;
                if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0 && free_last_directb(&dir_inode))
                {
                    printf("释放目录块失败！\n");
                    return 0xfffffffd;
                }
                if (dir_inode.size % DIR_ENTRY_PER_INDIRECT_BLOCK == 0 && free_last_indirectb(&dir_inode))
                {
                    printf("释放中间目录块失败！\n");
                    return 0xfffffffd;
                }
                if (dir_inode.size % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0 && free_last_double_indirectb(&dir_inode))
                {
                    printf("释放二级中间目录块失败！\n");
                    return 0xfffffffd;
                }
                if (dir_inode.size % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0 && free_last_triple_indirectb(&dir_inode))
                {
                    printf("释放三级目录块失败！\n");
                    return 0xfffffffd;
                }
                if (iwrite(dir_ino, &dir_inode))
                {
                    printf("写入 inode 失败！\n");
                    return 0xfffffffd;
                }
            }
            return ino;
        }
    }
    return 0xffffffff;
}

direntry_t get_last_direntry(const inode_t *const dir_inode)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;
    direntry_t de;
    memset(&de, 0, DIR_ENTRY_SIZE);

    if (dir_inode->size <= DIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->direct_blocks[(dir_inode->size - 1) / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
        {
            printf("读取目录块失败！\n");
            return de;
        }
    }
    else if (dir_inode->size - INDIRECT_DIR_OFFSET <= INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(
                dir_inode->indirect_blocks[(dir_inode->size - 1 - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return de;
        }
        if (bread(indirectb.blocks[(dir_inode->size - 1 - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIR_ENTRY_PER_DIRECT_BLOCK],
                  &db))
        {
            printf("读取目录块失败！\n");
            return de;
        }
    }
    else if ((dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) <= DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->double_indirect_blocks[(dir_inode->size - 1 - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return de;
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - 1 - DOUBLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return de;
        }
        if (bread(indirectb.blocks[(dir_inode->size - 1 - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIR_ENTRY_PER_DIRECT_BLOCK],
                  &db))
        {
            printf("读取目录块失败！\n");
            return de;
        }
    }
    else
    {
        if (bread(dir_inode->triple_indirect_blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indiectb))
        {
            printf("读取三级目录中间块失败！\n");
            return de;
        }
        if (bread(triple_indiectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return de;
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return de;
        }
        if (bread(indirectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIR_ENTRY_PER_DIRECT_BLOCK],
                  &db))
        {
            printf("读取目录块失败！\n");
            return de;
        }
    }
    return db.direntries[(dir_inode->size - 1) % DIR_ENTRY_PER_DIRECT_BLOCK];
}

int create_file(uint32_t dir_ino, uint32_t uid, uint32_t type, const char filename[])
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return -2;
    }
    assert(dir_inode.type == 1);
    if (dir_inode.size ==
        DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT + TRIPLE_INDIRECT_DIR_ENTRY_CNT)
        return -1;

    uint32_t db_bno;
    dirblock_t db;
    if (dir_inode.size + 1 < DIRECT_DIR_ENTRY_CNT)
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
    else if (dir_inode.size + 1 - INDIRECT_DIR_OFFSET < INDIRECT_DIR_ENTRY_CNT)
    {
        indirectblock_t indirectb;
        if ((dir_inode.size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if ((dir_inode.indirect_blocks[(dir_inode.size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
        }
        if (bread(dir_inode.indirect_blocks[(dir_inode.size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -2;
        }
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((indirectb.blocks[(dir_inode.size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(dir_inode.indirect_blocks[(dir_inode.size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -2;
            }
            sbwrite();
        }
        db_bno = indirectb.blocks[(dir_inode.size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -2;
        }
    }
    else if (dir_inode.size + 1 - DOUBLE_INDIRECT_DIR_OFFSET < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        indirectblock_t double_indirectb;
        if ((dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if ((dir_inode.double_indirect_blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                  DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
        }
        if (bread(dir_inode.double_indirect_blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                   DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级中间目录块失败！\n");
            return -2;
        }
        indirectblock_t indirectb;
        if ((dir_inode.size + 1 - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if ((double_indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(dir_inode.double_indirect_blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                        DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                       &double_indirectb))
            {
                printf("写入二级中间目录块失败！\n");
                return -2;
            }
            sbwrite();
        }
        if (bread(double_indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -2;
        }
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(double_indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -2;
            }
            sbwrite();
        }
        db_bno = indirectb.blocks[(dir_inode.size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
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
        if ((dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
        {
            if ((dir_inode.triple_indirect_blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                  DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
        }
        if (bread(dir_inode.triple_indirect_blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                   DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indirectb))
        {
            printf("读取三级中间目录块失败！\n");
            return -2;
        }
        indirectblock_t double_indirectb;
        if ((dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if ((triple_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(dir_inode.triple_indirect_blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                        DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                       &triple_indirectb))
            {
                printf("写入三级中间目录块失败！\n");
                return -2;
            }
            sbwrite();
        }
        if (bread(triple_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级中间目录块失败！\n");
            return -2;
        }
        indirectblock_t indirectb;
        if ((dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if ((double_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                     get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(
                    triple_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                            DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                    &double_indirectb))
            {
                printf("写入二级中间目录块失败！\n");
                return -2;
            }
            sbwrite();
        }
        if (bread(double_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -2;
        }
        if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if ((indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data()) == 0xffffffff)
            {
                printf("获取空闲数据块失败！\n");
                return -2;
            }
            if (bwrite(double_indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -2;
            }
            sbwrite();
        }
        db_bno = indirectb.blocks[(dir_inode.size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
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
    if (inode.type == 0)
    {
        inode.size = 0;
        inode.bcnt = 0;
    }
    else
    {
        assert(inode.type == 1);
        inode.size = 2;
        inode.bcnt = 1;
    }
    inode.uid = uid;
    inode.ctime = (uint32_t)time(NULL);
    inode.wtime = (uint32_t)time(NULL);
    inode.privilege = 066;
    if (inode.type == 1)
    {
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
        new_db.direntries[0].type = 1;
        new_db.direntries[1].ino = dir_ino;
        new_db.direntries[1].type = 1;
        strcpy(new_db.direntries[0].name, ".");
        strcpy(new_db.direntries[1].name, "..");
        if (bwrite(inode.direct_blocks[0], &new_db))
        {
            printf("写入目录块失败！\n");
            return -2;
        }
        sbwrite();
    }
    if (bwrite(db_bno, &db))
    {
        printf("写入目录块失败！\n");
        return -2;
    }
    sbwrite();
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

int remove_file(uint32_t ino, uint32_t uid)
{
    inode_t inode;
    if (iread(ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -3;
    }
    if (inode.uid != 0 && inode.uid != uid && (inode.privilege & 002) == 0)
        return -1;
    if (inode.type == 1)
        return -2;

    // for (uint32_t i = 0; i < inode.bcnt;)
    // {
    //     if (i < DIRECT_BLOCK_CNT)
    //         unset_data_bitmap(inode.direct_blocks[i++]);
    //     else if (i - INDIRECT_DATA_BLOCK_OFFSET < INDIRECT_DATA_BLOCK_CNT)
    //     {
    //         indirectblock_t indirectb;
    //     }
    //     else if ((j - DOUBLE_INDIRECT_DIR_OFFSET) < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    //     {
    //         if ((j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
    //             if (bread(dir_inode.double_indirect_blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) /
    //                                                        DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
    //                       &double_indirectb))
    //             {
    //                 printf("读取目录中间块失败！\n");
    //                 return 0xfffffffe;
    //             }
    //         if ((j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
    //             if (bread(double_indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
    //                       &indirectb))
    //             {
    //                 printf("读取目录中间块失败！\n");
    //                 return 0xfffffffe;
    //             }
    //         if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) / DIRECT_DIR_ENTRY_CNT], &db))
    //         {
    //             printf("读取目录块失败！\n");
    //             return 0xfffffffe;
    //         }
    //     }
    //     else if ((j - TRIPLE_INDIRECT_DIR_OFFSET) < TRIPLE_INDIRECT_DIR_ENTRY_CNT)
    //     {
    //         if ((j - TRIPLE_INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
    //             if (bread(dir_inode.triple_indirect_blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) /
    //                                                        DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
    //                       &triple_indiectb))
    //             {
    //                 printf("读取目录中间块失败！\n");
    //                 return 0xfffffffe;
    //             }
    //         if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
    //             if (bread(
    //                     triple_indiectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) /
    //                     DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK], &double_indirectb))
    //             {
    //                 printf("读取目录中间块失败！\n");
    //                 return 0xfffffffe;
    //             }
    //         if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
    //             if (bread(double_indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
    //                       &indirectb))
    //             {
    //                 printf("读取目录中间块失败！\n");
    //                 return 0xfffffffe;
    //             }
    //         if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) / DIRECT_DIR_ENTRY_CNT], &db))
    //         {
    //             printf("读取目录块失败！\n");
    //             return 0xfffffffe;
    //         }
    //     }
    // }
    // if (!strcmp(db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name, filename))
    // {
    //     if (type != NULL)
    //         *type = db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].type;
    //     return db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].ino;
    //}

    return 0;
}

int remove_dir(uint32_t ino, uint32_t uid)
{
    inode_t inode;
    if (iread(ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -3;
    }

    if (inode.uid != 0 && inode.uid != uid && (inode.privilege & 002) == 0)
        return -1;
    if (inode.type == 0)
        return -2;

    assert(inode.size >= 2);
    if (inode.size == 2) // 目录为空。
    {
        if (unset_data_bitmap(inode.direct_blocks[0]))
        {
            printf("修改数据块 bitmap 失败！\n");
            return -3;
        }
        if (unset_inode_bitmap(ino))
        {
            printf("修改 inode bitmap 失败！\n");
            return -3;
        }
    }
    else
    {
        indirectblock_t indirectb, double_indirectb, triple_indiectb;
        dirblock_t db;
        uint32_t dbno = 0, indirectbno = 0, double_indirectbno = 0, triple_indirectbno = 0;
        if (bread(inode.direct_blocks[0], &db))
        {
            printf("读取目录块失败！\n");
            return -3;
        }
        dbno = inode.direct_blocks[0];
        for (uint32_t i = 2; i < inode.size; i++)
        {
            if (i % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
            {
                if (dbno && unset_data_bitmap(dbno))
                {
                    printf("修改数据块位图失败！\n");
                    return -3;
                }
                if (i < DIRECT_DIR_ENTRY_CNT)
                {
                    if (bread(inode.direct_blocks[i / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
                    {
                        printf("读取目录块失败！\n");
                        return -3;
                    }
                    dbno = inode.direct_blocks[i / DIR_ENTRY_PER_DIRECT_BLOCK];
                }
                else if (i - INDIRECT_DIR_OFFSET < INDIRECT_DIR_ENTRY_CNT)
                {
                    if ((i - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    {
                        if (indirectbno && unset_data_bitmap(indirectbno))
                        {
                            printf("修改数据块位图失败！\n");
                            return -3;
                        }
                        if (bread(inode.indirect_blocks[(i - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                                  &indirectb))
                        {
                            printf("读取目录中间块失败！\n");
                            return -3;
                        }
                        indirectbno = inode.indirect_blocks[(i - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK];
                    }
                    if (bread(indirectb.blocks[(i - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                               DIRECT_DIR_ENTRY_CNT],
                              &db))
                    {
                        printf("读取目录块失败！\n");
                        return -3;
                    }
                    dbno = indirectb
                               .blocks[(i - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIRECT_DIR_ENTRY_CNT];
                }
                else if ((i - DOUBLE_INDIRECT_DIR_OFFSET) < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
                {
                    if ((i - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    {
                        if (double_indirectbno && unset_data_bitmap(double_indirectbno))
                        {
                            printf("修改数据块位图失败！\n");
                            return -3;
                        }
                        if (bread(inode.double_indirect_blocks[(i - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                                  &double_indirectb))
                        {
                            printf("读取二级目录中间块失败！\n");
                            return -3;
                        }
                        double_indirectbno = inode.double_indirect_blocks[(i - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK];
                    }
                    if ((i - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    {
                        if (indirectbno && unset_data_bitmap(indirectbno))
                        {
                            printf("修改数据块位图失败！\n");
                            return -3;
                        }
                        if (bread(double_indirectb
                                      .blocks[(i - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK /
                                              DIR_ENTRY_PER_INDIRECT_BLOCK],
                                  &indirectb))
                        {
                            printf("读取目录中间块失败！\n");
                            return -3;
                        }
                        indirectbno =
                            double_indirectb.blocks[(i - DOUBLE_INDIRECT_DIR_OFFSET) %
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK];
                    }
                    if (bread(indirectb.blocks[(i - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                               DIRECT_DIR_ENTRY_CNT],
                              &db))
                    {
                        printf("读取目录块失败！\n");
                        return -3;
                    }
                    dbno = indirectb.blocks[(i - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                            DIRECT_DIR_ENTRY_CNT];
                }
                else if ((i - TRIPLE_INDIRECT_DIR_OFFSET) < TRIPLE_INDIRECT_DIR_ENTRY_CNT)
                {
                    if ((i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
                    {
                        if (triple_indirectbno && unset_data_bitmap(triple_indirectbno))
                        {
                            printf("修改数据块位图失败！\n");
                            return -3;
                        }
                        if (bread(inode.triple_indirect_blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                                  &triple_indiectb))
                        {
                            printf("读取三级目录中间块失败！\n");
                            return -3;
                        }
                        triple_indirectbno = inode.triple_indirect_blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                                          DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK];
                    }
                    if ((i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    {
                        if (double_indirectbno && unset_data_bitmap(double_indirectbno))
                        {
                            printf("修改数据块位图失败！\n");
                            return -3;
                        }
                        if (bread(triple_indiectb
                                      .blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK /
                                              DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                                  &double_indirectb))
                        {
                            printf("读取二级目录中间块失败！\n");
                            return -3;
                        }
                        double_indirectbno =
                            triple_indiectb
                                .blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK];
                    }
                    if ((i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    {
                        if (indirectbno && unset_data_bitmap(indirectbno))
                        {
                            printf("修改数据块位图失败！\n");
                            return -3;
                        }
                        if (bread(double_indirectb
                                      .blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK /
                                              DIR_ENTRY_PER_INDIRECT_BLOCK],
                                  &indirectb))
                        {
                            printf("读取目录中间块失败！\n");
                            return -3;
                        }
                        indirectbno =
                            double_indirectb.blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) %
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK];
                    }
                    if (bread(indirectb.blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                               DIRECT_DIR_ENTRY_CNT],
                              &db))
                    {
                        printf("读取目录块失败！\n");
                        return -3;
                    }
                    dbno = indirectb.blocks[(i - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                            DIRECT_DIR_ENTRY_CNT];
                }
            }
            direntry_t de = db.direntries[i % DIR_ENTRY_PER_DIRECT_BLOCK];
            int ret = 0;
            if (de.type == 0)
            {
                if ((ret = remove_file(de.ino, uid)))
                    return ret;
            }
            else if ((ret = remove_dir(de.ino, uid))) // 递归删除子目录
                return ret;
        }
        if (dbno && unset_data_bitmap(dbno))
        {
            printf("修改数据块位图失败！\n");
            return -3;
        }
        if (indirectbno && unset_data_bitmap(indirectbno))
        {
            printf("修改数据块位图失败！\n");
            return -3;
        }
        if (double_indirectbno && unset_data_bitmap(double_indirectbno))
        {
            printf("修改数据块位图失败！\n");
            return -3;
        }
        if (triple_indirectbno && unset_data_bitmap(triple_indirectbno))
        {
            printf("修改数据块位图失败！\n");
            return -3;
        }
        if (unset_inode_bitmap(ino))
        {
            printf("修改 inode 位图失败！\n");
            return -3;
        }
    }
    return 0;
}