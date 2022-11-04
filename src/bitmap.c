#include "bitmap.h"
#include "assert.h"
#include "base_fun.h"
#include "device.h"
#include "format.h"
#include "stdio.h"
#include "string.h"

int set_inode_bitmap(uint32_t pos)
{
    assert(pos < sb.icnt);

    bitblock_t bb;
    pthread_rwlock_wrlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
    if (bread(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("读取位图失败！\n");
        return -1;
    }
    if (test_bitblock(&bb, pos % BIT_PER_BLOCK))
    {
        pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
        return 0;
    }
    set_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("写入位图失败！\n");
        return -1;
    }
    pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
    pthread_rwlock_wrlock(sb_lock);
    sb.free_icnt--;
    sbwrite(true);
    pthread_rwlock_unlock(sb_lock);

    return 0;
}

int set_data_bitmap(uint32_t pos)
{
    pos -= sb.data_start;
    assert(pos < sb.data_bcnt);

    bitblock_t bb;
    pthread_rwlock_wrlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
    if (bread(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("读取位图失败！\n");
        return -1;
    }
    if (test_bitblock(&bb, pos % BIT_PER_BLOCK))
    {
        pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
        return 0;
    }
    set_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("写入位图失败！\n");
        return -1;
    }
    pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
    pthread_rwlock_wrlock(sb_lock);
    sb.free_data_bcnt--;
    sbwrite(true);
    pthread_rwlock_unlock(sb_lock);

    return 0;
}

int unset_inode_bitmap(uint32_t pos)
{
    assert(pos < sb.icnt);

    bitblock_t bb;
    pthread_rwlock_wrlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
    if (bread(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("读取位图失败！\n");
        return -1;
    }
    if (!test_bitblock(&bb, pos % BIT_PER_BLOCK))
    {
        pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
        return 0;
    }
    unset_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.inode_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("写入位图失败！\n");
        return -1;
    }
    pthread_rwlock_unlock(inode_bitmap_locks[pos / BIT_PER_BLOCK]);
    pthread_rwlock_wrlock(sb_lock);
    sb.free_icnt++;
    sbwrite(true);
    pthread_rwlock_unlock(sb_lock);

    return 0;
}

int unset_data_bitmap(uint32_t pos)
{
    pos -= sb.data_start;
    assert(pos < sb.data_bcnt);

    bitblock_t bb;
    pthread_rwlock_wrlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
    if (bread(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("读取位图失败！\n");
        return -1;
    }
    if (!test_bitblock(&bb, pos % BIT_PER_BLOCK))
    {
        pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
        return 0;
    }
    unset_bitblock(&bb, pos % BIT_PER_BLOCK);
    if (bwrite(sb.data_bitmap_start + pos / BIT_PER_BLOCK, &bb))
    {
        pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
        printf("写入位图失败！\n");
        return -1;
    }
    pthread_rwlock_unlock(data_bitmap_locks[pos / BIT_PER_BLOCK]);
    pthread_rwlock_wrlock(sb_lock);
    sb.free_data_bcnt++;
    sbwrite(true);
    pthread_rwlock_unlock(sb_lock);

    return 0;
}

// 偏移为 3 的位运算即为乘 8 或除 8 。

bool test_bitblock(const bitblock_t *const bb, uint32_t pos)
{
    assert(pos < BIT_PER_BLOCK);
    return bb->bytes[pos >> 3] & (1 << (pos & 7));
}

void set_bitblock(bitblock_t *const bb, uint32_t pos)
{
    assert(pos < BIT_PER_BLOCK);
    bb->bytes[pos >> 3] |= 1 << (pos & 7);
}

void unset_bitblock(bitblock_t *const bb, uint32_t pos)
{
    assert(pos < BIT_PER_BLOCK);
    bb->bytes[pos >> 3] ^= 1 << (pos & 7);
}

uint32_t get_free_inode()
{
    if (sb.free_icnt == 0)
    {
        printf("空闲 inode 数量不足，无法分配！\n");
        return -1;
    }

    bitblock_t bb;
    uint32_t ino;
    if (sb.last_alloc_inode < sb.icnt - 1)
        ino = sb.last_alloc_inode + 1;
    else
        ino = 1; // 从头分配，编号为 0 的 inode 为根目录。

    while (true)
    {
        pthread_rwlock_wrlock(inode_bitmap_locks[ino / BIT_PER_BLOCK]);
        if (bread(sb.inode_bitmap_start + ino / BIT_PER_BLOCK, &bb))
        {
            printf("读取位图失败！\n");
            return -2;
        }
        while (true)
        {
            if (!test_bitblock(&bb, ino % BIT_PER_BLOCK))
            {
                set_bitblock(&bb, ino % BIT_PER_BLOCK);
                if (bwrite(sb.inode_bitmap_start + ino / BIT_PER_BLOCK, &bb))
                {
                    pthread_rwlock_unlock(inode_bitmap_locks[ino / BIT_PER_BLOCK]);
                    printf("写入位图失败！\n");
                    return -2;
                }
                pthread_rwlock_unlock(inode_bitmap_locks[ino / BIT_PER_BLOCK]);
                pthread_rwlock_wrlock(sb_lock);
                sb.free_icnt--;
                sb.last_alloc_inode = ino;
                sbwrite(true);
                pthread_rwlock_unlock(sb_lock);
                return ino;
            }
            if (ino < sb.icnt - 1)
                ino++;
            else
                ino = 0;

            if (ino % BIT_PER_BLOCK == 0) // 需要读取新的块。
            {
                pthread_rwlock_unlock(inode_bitmap_locks[(ino - 1) / BIT_PER_BLOCK]);
                break;
            }

            // 避免死循环，如果以下 if 分支被执行，说明文件系统出现了不一致。
            if (ino == sb.last_alloc_inode)
            {
                pthread_rwlock_unlock(inode_bitmap_locks[ino / BIT_PER_BLOCK]);
                printf("未找到空闲 inode ，无法分配！\n");
                return -2;
            }
        }
    }
}

uint32_t get_free_data()
{
    if (sb.free_data_bcnt == 0)
    {
        printf("空闲数据块数量不足，无法分配！\n");
        return -1;
    }

    bitblock_t bb;
    uint32_t bno; // 临时名称，返回时会加上数据块起始偏移量。
    if (sb.last_alloc_data < sb.data_bcnt - 1)
        bno = sb.last_alloc_data + 1;
    else
        bno = 1; // 从头分配，编号为 0 的数据块分配给了根目录。

    while (true)
    {
        pthread_rwlock_wrlock(data_bitmap_locks[bno / BIT_PER_BLOCK]);
        if (bread(sb.data_bitmap_start + bno / BIT_PER_BLOCK, &bb))
        {
            pthread_rwlock_unlock(data_bitmap_locks[bno / BIT_PER_BLOCK]);
            printf("读取位图失败！\n");
            return -2;
        }
        while (true)
        {
            if (!test_bitblock(&bb, bno % BIT_PER_BLOCK))
            {
                set_bitblock(&bb, bno % BIT_PER_BLOCK);
                if (bwrite(sb.data_bitmap_start + bno / BIT_PER_BLOCK, &bb))
                {
                    pthread_rwlock_unlock(data_bitmap_locks[bno / BIT_PER_BLOCK]);
                    printf("写入位图失败！\n");
                    return -2;
                }
                pthread_rwlock_unlock(data_bitmap_locks[bno / BIT_PER_BLOCK]);
                pthread_rwlock_wrlock(sb_lock);
                sb.free_data_bcnt--;
                sb.last_alloc_data = bno;
                sbwrite(true);
                pthread_rwlock_unlock(sb_lock);
                return sb.data_start + bno;
            }
            if (bno < sb.data_bcnt - 1)
                bno++;
            else
                bno = 0;

            if (bno % BIT_PER_BLOCK == 0) // 需要读取新的块。
            {
                pthread_rwlock_unlock(data_bitmap_locks[(bno - 1) / BIT_PER_BLOCK]);
                break;
            }

            // 避免死循环，如果以下 if 分支被执行，说明文件系统出现了不一致。
            if (bno == sb.last_alloc_data)
            {
                pthread_rwlock_unlock(data_bitmap_locks[bno / BIT_PER_BLOCK]);
                printf("未找到空闲数据块，无法分配！\n");
                return -2;
            }
        }
    }
}