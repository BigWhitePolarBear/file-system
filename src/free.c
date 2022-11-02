#include "free.h"
#include "assert.h"
#include "bitmap.h"
#include "device.h"
#include "stdio.h"

int free_last_directb(inode_t *const dir_inode)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;

    if (dir_inode->size <= DIRECT_DIR_ENTRY_CNT)
    {
        if (unset_data_bitmap(dir_inode->direct_blocks[dir_inode->size / DIR_ENTRY_PER_DIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    else if (dir_inode->size - INDIRECT_DIR_OFFSET <= INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -1;
        }
        if (unset_data_bitmap(indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                               DIR_ENTRY_PER_DIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    else if ((dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) <= DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -1;
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -1;
        }
        if (unset_data_bitmap(indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                               DIR_ENTRY_PER_DIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    else
    {
        if (bread(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indiectb))
        {
            printf("读取三级目录中间块失败！\n");
            return -1;
        }
        if (bread(triple_indiectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -1;
        }
        if (bread(double_indirectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -1;
        }
        if (unset_data_bitmap(indirectb.blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                               DIR_ENTRY_PER_DIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    return 0;
}

int free_last_indirectb(inode_t *const dir_inode)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size >= DIRECT_DIR_ENTRY_CNT);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t double_indirectb, triple_indiectb;

    if (dir_inode->size - INDIRECT_DIR_OFFSET <= INDIRECT_DIR_ENTRY_CNT)
    {
        if (unset_data_bitmap(
                dir_inode->indirect_blocks[(dir_inode->size - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    else if ((dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) <= DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if (bread(dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -1;
        }
        if (unset_data_bitmap(
                (double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK])))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    else
    {
        if (bread(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indiectb))
        {
            printf("读取三级目录中间块失败！\n");
            return -1;
        }
        if (bread(triple_indiectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                         DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -1;
        }
        if (unset_data_bitmap(
                double_indirectb.blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) %
                                        DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    return 0;
}

int free_last_double_indirectb(inode_t *const dir_inode)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size >= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t triple_indiectb;

    if ((dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) <= DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if (unset_data_bitmap((dir_inode->double_indirect_blocks[(dir_inode->size - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                                 DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK])))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    else
    {
        if (bread(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indiectb))
        {
            printf("读取三级目录中间块失败！\n");
            return -1;
        }
        if (unset_data_bitmap(
                triple_indiectb.blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) %
                                       DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK]))
        {
            printf("修改数据块位图失败！\n");
            return -1;
        }
    }
    return 0;
}

int free_last_triple_indirectb(inode_t *const dir_inode)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size >= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);

    if (unset_data_bitmap(dir_inode->triple_indirect_blocks[(dir_inode->size - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                            DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK]))
    {
        printf("修改数据块位图失败！\n");
        return -1;
    }

    return 0;
}