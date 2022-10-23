#include "fs.h"
#include "time.h"

struct super_block sb;

int mkfs()
{
    if (mksb())
        return -1;

    return 0;
}

int mksb()
{
    sb.status = 0;
    sb.format_time = (uint32_t)time(NULL);
    sb.last_wtime = (uint32_t)time(NULL);
    sb.bsize = BLOCK_SIZE;
    sb.bcnt = BLOCK_CNT;
    sb.free_bcnt = BLOCK_CNT - 1;
    sb.icnt = INODE_CNT;
    sb.free_icnt = INODE_CNT;
    sb.isize = 256;
    // 数据块数量，由于一共有 100k 个块，因此需要 12800B 储存
    // 位图，占用 13 个块，超级块占用一个块，一共减 14 。
    sb.data_bcnt = BLOCK_CNT - 14 - INODE_CNT * 256 / 1024;
    sb.free_data_bcnt = BLOCK_CNT - 14 - INODE_CNT * 256 / 1024;

    return bwrite(0, &sb);
}

int sb_update_last_wtime()
{
    sb.last_wtime = (uint32_t)time(NULL);

    return bwrite(0, &sb);
}