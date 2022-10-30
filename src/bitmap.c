#include "bitmap.h"
#include "assert.h"
#include "device.h"
#include "format.h"
#include "internal_fun.h"
#include "stdio.h"

int set_inode_bitmap(uint32_t pos)
{
    assert(pos < sb.icnt);

    bitblock_t bb;
    if (bread(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("读取位图失败！\n");
        return -1;
    }
    set_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("写入位图失败！\n");
        return -1;
    }
    sb.free_icnt--;
    sbwrite();

    return 0;
}

int set_data_bitmap(uint32_t pos)
{
    assert(pos < sb.data_bcnt);

    bitblock_t bb;
    if (bread(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("读取位图失败！\n");
        return -1;
    }
    set_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("写入位图失败！\n");
        return -1;
    }
    sb.free_data_bcnt--;
    sbwrite();

    return 0;
}

int unset_inode_bitmap(uint32_t pos)
{
    assert(pos < sb.icnt);

    bitblock_t bb;
    if (bread(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("读取位图失败！\n");
        return -1;
    }
    unset_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("写入位图失败！\n");
        return -1;
    }
    sb.free_icnt++;
    sbwrite();

    return 0;
}

int unset_data_bitmap(uint32_t pos)
{
    assert(pos < sb.data_bcnt);

    bitblock_t bb;
    if (bread(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("读取位图失败！\n");
        return -1;
    }
    unset_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        printf("写入位图失败！\n");
        return -1;
    }
    sb.free_data_bcnt++;
    sbwrite();

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

void unset_bitblock(bitblock_t *bb, uint32_t pos)
{
    // 偏移为 3 的位运算即为乘 8 或除 8 。
    assert(pos < BIT_PER_BLOCK);
    bb->bytes[pos >> 3] ^= 1 << (pos & 7);
}