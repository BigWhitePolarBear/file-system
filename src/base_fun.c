#include "base_fun.h"
#include "assert.h"
#include "bitmap.h"
#include "common.h"
#include "device.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"

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

bool check_privilege(const inode_t *const inode, uint32_t uid, uint32_t privilege)
{
    if (uid == inode->uid)
        return privilege & (inode->privilege >> 3);
    return privilege & inode->privilege;
}

int _push_direntry(inode_t *const dir_inode, const direntry_t *const de)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    if (dir_inode->size ==
        DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT + TRIPLE_INDIRECT_DIR_ENTRY_CNT)
        return -1;

    uint32_t db_bno;
    dirblock_t db;
    if (dir_inode->size + 1 < DIRECT_DIR_ENTRY_CNT)
    {
        if (dir_inode->size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            dir_inode->direct_blocks[dir_inode->size / DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data();
            switch (dir_inode->direct_blocks[dir_inode->size / DIR_ENTRY_PER_DIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(dir_inode->direct_blocks[dir_inode->size / DIR_ENTRY_PER_DIRECT_BLOCK] < (uint32_t)-2);
                break;
            }
            dir_inode->bcnt++;
        }
        db_bno = dir_inode->direct_blocks[dir_inode->size / DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -3;
        }
    }
    else if (dir_inode->size + 1 - INDIRECT_DIR_OFFSET < INDIRECT_DIR_ENTRY_CNT)
    {
        indirectblock_t indirectb;
        if ((dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                get_free_data();
            switch (dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(
                    dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK] <
                    (uint32_t)-2);
                break;
            }
        }
        if (bread(dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -5;
        }
        if (dir_inode->size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                             DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data();
            switch (indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                     DIR_ENTRY_PER_DIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DIRECT_BLOCK] < (uint32_t)-2);
                break;
            }
            if (bwrite(
                    dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                    &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -3;
            }
            sbwrite();
        }
        db_bno = indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -3;
        }
    }
    else if (dir_inode->size + 1 - DOUBLE_INDIRECT_DIR_OFFSET < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        indirectblock_t double_indirectb;
        if ((dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                              DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] = get_free_data();
            switch (dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                      DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] < (uint32_t)-2);
                break;
            }
        }
        if (bread(dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级中间目录块失败！\n");
            return -3;
        }
        indirectblock_t indirectb;
        if ((dir_inode->size + 1 - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                get_free_data();
            switch (double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                            DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] <
                       (uint32_t)-2);
                break;
            }
            if (bwrite(dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                       &double_indirectb))
            {
                printf("写入二级中间目录块失败！\n");
                return -3;
            }
            sbwrite();
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -3;
        }
        if (dir_inode->size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                             DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data();
            switch (indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                     DIR_ENTRY_PER_DIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DIRECT_BLOCK] < (uint32_t)-2);
                break;
            }
            if (bwrite(double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -3;
            }
            sbwrite();
        }
        db_bno = indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -3;
        }
    }
    else
    {
        indirectblock_t triple_indirectb;
        if ((dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
        {
            dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                              DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK] = get_free_data();
            switch (dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                      DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK] < (uint32_t)-2);
                break;
            }
        }
        if (bread(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indirectb))
        {
            printf("读取三级中间目录块失败！\n");
            return -3;
        }
        indirectblock_t double_indirectb;
        if ((dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            triple_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] =
                get_free_data();
            switch (triple_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                            DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(
                    triple_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                            DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] <
                    (uint32_t)-2);
                break;
            }
            if (bwrite(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                       &triple_indirectb))
            {
                printf("写入三级中间目录块失败！\n");
                return -3;
            }
            sbwrite();
        }
        if (bread(triple_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级中间目录块失败！\n");
            return -3;
        }
        indirectblock_t indirectb;
        if ((dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            double_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                get_free_data();
            switch (double_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                            DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(double_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] <
                       (uint32_t)-2);
                break;
            }
            if (bwrite(
                    triple_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                            DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                    &double_indirectb))
            {
                printf("写入二级中间目录块失败！\n");
                return -3;
            }
            sbwrite();
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取中间目录块失败！\n");
            return -3;
        }
        if (dir_inode->size % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                             DIR_ENTRY_PER_DIRECT_BLOCK] = get_free_data();
            switch (indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                     DIR_ENTRY_PER_DIRECT_BLOCK])
            {
            case -1:
                return -2;
                break;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -3;
            default:
                assert(indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                        DIR_ENTRY_PER_DIRECT_BLOCK] < (uint32_t)-2);
                break;
            }
            if (bwrite(double_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                       &indirectb))
            {
                printf("写入中间目录块失败！\n");
                return -3;
            }
            sbwrite();
        }
        db_bno = indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                  DIR_ENTRY_PER_DIRECT_BLOCK];
        if (bread(db_bno, &db))
        {
            printf("读取目录块失败！\n");
            return -3;
        }
    }

    db.direntries[dir_inode->size % DIR_ENTRY_PER_DIRECT_BLOCK] = *de;
    if (bwrite(db_bno, &db))
    {
        printf("写入目录块失败！\n");
        return -3;
    }

    return 0;
}

int _get_last_direntry(const inode_t *const dir_inode, direntry_t *const de)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;

    if (dir_inode->size <= DIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->direct_blocks[(dir_inode->size - 1) / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
        {
            printf("读取目录块失败！\n");
            return -1;
        }
    }
    else if (dir_inode->size - INDIRECT_DIR_OFFSET <= INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(
                dir_inode->indirect_blocks[(dir_inode->size - 1 - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -1;
        }
        if (bread(indirectb.blocks[(dir_inode->size - 1 - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIR_ENTRY_PER_DIRECT_BLOCK],
                  &db))
        {
            printf("读取目录块失败！\n");
            return -1;
        }
    }
    else if ((dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) <= DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->double_indirect_blocks[(dir_inode->size - 1 - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -1;
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - 1 - DOUBLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -1;
        }
        if (bread(indirectb.blocks[(dir_inode->size - 1 - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIR_ENTRY_PER_DIRECT_BLOCK],
                  &db))
        {
            printf("读取目录块失败！\n");
            return -1;
        }
    }
    else
    {
        if (bread(dir_inode->triple_indirect_blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indiectb))
        {
            printf("读取三级目录中间块失败！\n");
            return -1;
        }
        if (bread(triple_indiectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -1;
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -1;
        }
        if (bread(indirectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIR_ENTRY_PER_DIRECT_BLOCK],
                  &db))
        {
            printf("读取目录块失败！\n");
            return -1;
        }
    }
    *de = db.direntries[(dir_inode->size - 1) % DIR_ENTRY_PER_DIRECT_BLOCK];
    return 0;
}