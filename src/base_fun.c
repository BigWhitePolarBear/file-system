#include "base_fun.h"
#include "assert.h"
#include "bitmap.h"
#include "common.h"
#include "device.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

pthread_rwlock_t **inode_bitmap_locks, **data_bitmap_locks;
pthread_rwlock_t *inode_lock, *data_lock, *sb_lock;

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

void lock_init()
{
    inode_bitmap_locks = malloc(sizeof(pthread_rwlock_t *) * INODE_BITMAP_BCNT);
    for (uint32_t i = 0; i < INODE_BITMAP_BCNT; i++)
    {
        inode_bitmap_locks[i] = malloc(sizeof(pthread_rwlock_t));
        if (pthread_rwlock_init(inode_bitmap_locks[i], NULL))
        {
            printf("锁初始化失败！\n");
            printf("致命错误，退出系统！\n");
            exit(-1);
        }
    }

    data_bitmap_locks = malloc(sizeof(pthread_rwlock_t *) * DATA_BITMAP_BCNT);
    for (uint32_t i = 0; i < DATA_BITMAP_BCNT; i++)
    {
        data_bitmap_locks[i] = malloc(sizeof(pthread_rwlock_t));
        if (pthread_rwlock_init(data_bitmap_locks[i], NULL))
        {
            printf("锁初始化失败！\n");
            printf("致命错误，退出系统！\n");
            exit(-1);
        }
    }

    inode_lock = malloc(sizeof(pthread_rwlock_t));
    data_lock = malloc(sizeof(pthread_rwlock_t));
    sb_lock = malloc(sizeof(pthread_rwlock_t));
    if (pthread_rwlock_init(inode_lock, NULL) || pthread_rwlock_init(data_lock, NULL) ||
        pthread_rwlock_init(sb_lock, NULL))
    {
        printf("锁初始化失败！\n");
        printf("致命错误，退出系统！\n");
        exit(-1);
    }
}

void sbwrite()
{
    pthread_rwlock_wrlock(sb_lock);
    sb.last_wtime = (uint32_t)time(NULL);
    if (bwrite(0, &sb))
    {
        printf("写入超级块失败！\n");
        printf("致命错误，退出系统！\n");
        exit(-1);
    }
    pthread_rwlock_unlock(sb_lock);
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
    dir_inode->size++;

    return 0;
}

int _get_last_direntry(const inode_t *const dir_inode, direntry_t *const de)
{
    assert(dir_inode->type == 1);
    assert(dir_inode->size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                  TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    indirectblock_t indirectb, double_indirectb, triple_indirectb;
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
                  &triple_indirectb))
        {
            printf("读取三级目录中间块失败！\n");
            return -1;
        }
        if (bread(triple_indirectb.blocks[(dir_inode->size - 1 - TRIPLE_INDIRECT_DIR_OFFSET) %
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

int _read_file(const inode_t *const inode, uint32_t page, datablock_t *const db)
{
    assert(inode->type == 0);
    assert(inode->bcnt <= DIRECT_DATA_BLOCK_CNT + INDIRECT_DATA_BLOCK_CNT + DOUBLE_INDIRECT_DATA_BLOCK_CNT +
                              TRIPLE_INDIRECT_DATA_BLOCK_CNT);

    assert(page <= inode->bcnt);

    indirectblock_t indirectb, double_indirectb, triple_indirectb;
    if (page < DIRECT_DATA_BLOCK_CNT)
    {
        if (bread(inode->direct_blocks[page], db))
        {
            printf("读取数据块失败！\n");
            return -1;
        }
    }
    else if (page - INDIRECT_DATA_BLOCK_OFFSET < INDIRECT_DATA_BLOCK_CNT)
    {
        if ((page - INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK == 0)
        {
            if (bread(inode->indirect_blocks[(page - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK], &indirectb))
            {
                printf("读取目录中间块失败！\n");
                return -1;
            }
        }
        if (bread(indirectb.blocks[(page - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIRECT_DIR_ENTRY_CNT],
                  db))
        {
            printf("读取数据块失败！\n");
            return -1;
        }
    }
    else if ((page - DOUBLE_INDIRECT_DIR_OFFSET) < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if ((page - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if (bread(inode->double_indirect_blocks[(page - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                      &double_indirectb))
            {
                printf("读取二级目录中间块失败！\n");
                return -1;
            }
        }
        if ((page - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if (bread(double_indirectb.blocks[(page - DOUBLE_INDIRECT_DIR_OFFSET) %
                                              DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                      &indirectb))
            {
                printf("读取目录中间块失败！\n");
                return -1;
            }
        }
        if (bread(indirectb.blocks[(page - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIRECT_DIR_ENTRY_CNT],
                  db))
        {
            printf("读取数据块失败！\n");
            return -1;
        }
    }
    else
    {
        if ((page - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
        {
            if (bread(inode->triple_indirect_blocks[(page - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                    DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                      &triple_indirectb))
            {
                printf("读取三级目录中间块失败！\n");
                return -1;
            }
        }
        if ((page - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if (bread(
                    triple_indirectb.blocks[(page - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK /
                                            DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                    &double_indirectb))
            {
                printf("读取二级目录中间块失败！\n");
                return -1;
            }
        }
        if ((page - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if (bread(double_indirectb.blocks[(page - TRIPLE_INDIRECT_DIR_OFFSET) %
                                              DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                      &indirectb))
            {
                printf("读取目录中间块失败！\n");
                return -1;
            }
        }
        if (bread(indirectb.blocks[(page - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                   DIRECT_DIR_ENTRY_CNT],
                  db))
        {
            printf("读取数据块失败！\n");
            return -1;
        }
    }
    return 0;
}

int _append_block(inode_t *const inode, const datablock_t *const db)
{
    assert(inode->type == 0);
    assert(inode->bcnt < DIRECT_DATA_BLOCK_CNT + INDIRECT_DATA_BLOCK_CNT + DOUBLE_INDIRECT_DATA_BLOCK_CNT +
                             TRIPLE_INDIRECT_DATA_BLOCK_CNT);

    indirectblock_t indirectb, double_indirectb, triple_indirectb;
    memset(&indirectb, 0, BLOCK_SIZE);
    memset(&double_indirectb, 0, BLOCK_SIZE);
    memset(&triple_indirectb, 0, BLOCK_SIZE);
    uint32_t indirectbno = 0, double_indirectbno = 0, triple_indirectbno = 0;
    if (inode->bcnt < DIRECT_DATA_BLOCK_CNT)
    {
        inode->direct_blocks[inode->bcnt] = get_free_data();
        switch (inode->direct_blocks[inode->bcnt])
        {
        case -1:
            return -1;
        case -2:
            printf("获取空闲数据块失败！\n");
            return -2;
        default:
            break;
        }
        if (bwrite(inode->direct_blocks[inode->bcnt], db))
        {
            printf("写入数据块失败！\n");
            return -2;
        }
    }
    else if (inode->bcnt - INDIRECT_DATA_BLOCK_OFFSET < INDIRECT_DATA_BLOCK_CNT)
    {
        if ((inode->bcnt - INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK == 0)
        {
            if (indirectbno && bwrite(indirectbno, &indirectb))
            {
                printf("写入目录中间块失败！\n");
                return -2;
            }
            indirectbno = inode->indirect_blocks[(inode->bcnt - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                get_free_data();
            switch (indirectbno)
            {
            case -1:
                return -1;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -2;
            default:
                break;
            }
            if (bread(indirectbno, &indirectb))
            {
                printf("读取目录中间块失败！\n");
                return -2;
            }
        }
        indirectb.blocks[(inode->bcnt - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIRECT_DIR_ENTRY_CNT] =
            get_free_data();
        switch (
            indirectb.blocks[(inode->bcnt - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIRECT_DIR_ENTRY_CNT])
        {
        case -1:
            return -1;
        case -2:
            printf("获取空闲数据块失败！\n");
            return -2;
        default:
            break;
        }
        if (bwrite(indirectb.blocks[(inode->bcnt - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                    DIRECT_DIR_ENTRY_CNT],
                   db))
        {
            printf("写入数据块失败！\n");
            return -2;
        }
    }
    else if ((inode->bcnt - DOUBLE_INDIRECT_DIR_OFFSET) < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
    {
        if ((inode->bcnt - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if (double_indirectbno && bwrite(indirectbno, &double_indirectb))
            {
                printf("写入二级目录中间块失败！\n");
                return -2;
            }
            double_indirectbno = inode->double_indirect_blocks[(inode->bcnt - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] = get_free_data();
            switch (double_indirectbno)
            {
            case -1:
                return -1;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -2;
            default:
                break;
            }
            if (bread(double_indirectbno, &double_indirectb))
            {
                printf("读取二级目录中间块失败！\n");
                return -2;
            }
        }
        if ((inode->bcnt - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if (indirectbno && bwrite(indirectbno, &indirectb))
            {
                printf("写入目录中间块失败！\n");
                return -2;
            }
            indirectbno = double_indirectb.blocks[(inode->bcnt - DOUBLE_INDIRECT_DIR_OFFSET) %
                                                  DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                get_free_data();
            switch (indirectbno)
            {
            case -1:
                return -1;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -2;
            default:
                break;
            }
            if (bread(indirectbno, &indirectb))
            {
                printf("读取目录中间块失败！\n");
                return -2;
            }
        }
        if (bwrite(indirectb.blocks[(inode->bcnt - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                    DIRECT_DIR_ENTRY_CNT],
                   db))
        {
            printf("写入数据块失败！\n");
            return -2;
        }
    }
    else
    {
        if ((inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
        {
            if (triple_indirectbno && bwrite(triple_indirectbno, &triple_indirectb))
            {
                printf("写入三级目录中间块失败！\n");
                return -2;
            }
            triple_indirectbno = inode->triple_indirect_blocks[(inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK] = get_free_data();
            switch (triple_indirectbno)
            {
            case -1:
                return -1;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -2;
            default:
                break;
            }
            if (bread(triple_indirectbno, &triple_indirectb))
            {
                printf("读取三级目录中间块失败！\n");
                return -2;
            }
        }
        if ((inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
        {
            if (double_indirectbno && bwrite(indirectbno, &double_indirectb))
            {
                printf("写入二级目录中间块失败！\n");
                return -2;
            }
            double_indirectbno =
                triple_indirectb.blocks[(inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) %
                                        DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK] =
                    get_free_data();
            switch (double_indirectbno)
            {
            case -1:
                return -1;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -2;
            default:
                break;
            }
            if (bread(double_indirectbno, &double_indirectb))
            {
                printf("读取二级目录中间块失败！\n");
                return -2;
            }
        }
        if ((inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
        {
            if (indirectbno && bwrite(indirectbno, &indirectb))
            {
                printf("写入目录中间块失败！\n");
                return -2;
            }
            indirectbno = double_indirectb.blocks[(inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) %
                                                  DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK] =
                get_free_data();
            switch (indirectbno)
            {
            case -1:
                return -1;
            case -2:
                printf("获取空闲数据块失败！\n");
                return -2;
            default:
                break;
            }
            if (bread(indirectbno, &indirectb))
            {
                printf("读取目录中间块失败！\n");
                return -2;
            }
        }
        if (bwrite(indirectb.blocks[(inode->bcnt - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                    DIRECT_DIR_ENTRY_CNT],
                   db))
        {
            printf("读取数据块失败！\n");
            return -2;
        }
    }

    if (indirectbno && bwrite(indirectbno, &indirectb))
    {
        printf("写入目录中间块失败！\n");
        return -2;
    }
    if (double_indirectbno && bwrite(indirectbno, &double_indirectb))
    {
        printf("写入二级目录中间块失败！\n");
        return -2;
    }
    if (triple_indirectbno && bwrite(triple_indirectbno, &triple_indirectb))
    {
        printf("写入三级目录中间块失败！\n");
        return -2;
    }

    inode->bcnt++;
    inode->size += BLOCK_SIZE;

    return 0;
}