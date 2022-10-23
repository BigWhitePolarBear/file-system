#include "fs.h"
#include "assert.h"
#include "time.h"

superblock_t sb;

int mkfs()
{
    if (mksb())
        return -1;

    if (mkbitmap())
        return -1;

    return 0;
}

int mksb()
{
    sb.status = 0;
    sb.fmt_time = (uint32_t)time(NULL);
    sb.last_wtime = (uint32_t)time(NULL);
    sb.bsize = BLOCK_SIZE;
    sb.bcnt = BLOCK_CNT;
    sb.free_bcnt = BLOCK_CNT - 1;
    sb.icnt = INODE_CNT;
    sb.free_icnt = INODE_CNT;
    sb.isize = 128;
    sb.data_bcnt = BLOCK_CNT - 1 - 13 - INODE_CNT * 128 / 1024;
    sb.free_data_bcnt = BLOCK_CNT - 1 - 13 - INODE_CNT * 128 / 1024;

    return bwrite(0, &sb);
}

int sb_update_last_wtime()
{
    sb.last_wtime = (uint32_t)time(NULL);

    return bwrite(0, &sb);
}

int mkbitmap()
{
    // 需要 13 个块储存位图。
    bitblock_t bb;
    for (int i = 1; i <= 13; i++)
        if (bwrite(i, &bb))
            return -1;

    // 第一个位图的低 14 位置 1 。
    for (int pos = 0; pos < 14; pos++)
        set_bitblock(&bb, pos);
    return bwrite(1, &bb);
}

void set_bitblock(bitblock_t *bb, uint32_t pos)
{
    // 偏移为 3 的位运算即为乘 8 或除 8 。
    assert(pos < BLOCK_SIZE << 3 - 1);
    bb->bytes[pos >> 3] |= 1 << (pos & 7);
}
