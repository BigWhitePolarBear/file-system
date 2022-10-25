#include "assert.h"
#include "fs.h"

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
            return 0xffffffff;
        while (ino % BIT_PER_BLOCK > 0)
        {
            if (test_bitblock(&bb, ino % BIT_PER_BLOCK))
            {
                set_bitblock(&bb, ino % BIT_PER_BLOCK);
                if (bwrite(sb.inode_bitmap_start + ino / BIT_PER_BLOCK, &bb))
                {
                    printf("持久化 bitmap 失败！\n");
                    return 0xffffffff;
                }
                sb.free_icnt--;
                sb.last_alloc_inode = ino;
                if (bwrite(0, &sb))
                {
                    printf("持久化超级块失败！\n");
                    return 0xffffffff;
                }
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
        bno = 0;

    while (1)
    {
        if (bread(sb.data_bitmap_start + bno / BIT_PER_BLOCK, &bb))
            return 0xffffffff;
        while (bno % BIT_PER_BLOCK > 0)
        {
            if (test_bitblock(&bb, bno % BIT_PER_BLOCK))
            {
                set_bitblock(&bb, bno % BIT_PER_BLOCK);
                if (bwrite(sb.inode_bitmap_start + bno / BIT_PER_BLOCK, &bb))
                {
                    printf("持久化 bitmap 失败！\n");
                    return 0xffffffff;
                }
                sb.free_data_bcnt--;
                sb.last_alloc_data = bno;
                if (bwrite(0, &sb))
                {
                    printf("持久化超级块失败！\n");
                    return 0xffffffff;
                }
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

int set_inode_bitmap(uint32_t pos)
{
    bitblock_t bb;

    if (bread(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
        return -1;
    set_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
        return -1;

    return 0;
}

int set_data_bitmap(uint32_t pos)
{
    bitblock_t bb;

    if (bread(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
        return -1;
    set_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
        return -1;

    return 0;
}

uint8_t test_bitblock(bitblock_t *bb, uint32_t pos)
{
    // 偏移为 3 的位运算即为乘 8 或除 8 。
    assert(pos < BIT_PER_BLOCK);
    return bb->bytes[pos >> 3];
}

void set_bitblock(bitblock_t *bb, uint32_t pos)
{
    // 偏移为 3 的位运算即为乘 8 或除 8 。
    assert(pos < BIT_PER_BLOCK);
    bb->bytes[pos >> 3] |= 1 << (pos & 7);
}
