#include "internal_fun.h"
#include "assert.h"
#include "base_fun.h"
#include "bitmap.h"
#include "device.h"
#include "free.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "unistd.h"

int locate_last(uint8_t *l, uint8_t *r, uint32_t *working_dir, uint8_t cmd_len, const char cmd[])
{
    while (1)
    {
        while (*r < cmd_len && cmd[*r] != '/')
            (*r)++;
        if (*r < cmd_len)
        {
            if (*r - *l > FILE_NAME_LEN)
                return -1;
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd + *l, *r - *l);
            uint32_t type;
            *working_dir = search(*working_dir, 0, filename, &type, false, false); // uid 不重要，因为不会删除目录项。
            if (*working_dir == -4)
                return -2;
            else if (*working_dir == -1 || type != 1)
                return -1;
        }
        else // 说明已经是最后一个目录。
        {
            if (*r - *l > FILE_NAME_LEN)
                return -1;
            break;
        }
        (*r)++;
        *l = *r;
    }
    return 0;
}

uint32_t search(uint32_t dir_ino, uint32_t uid, const char filename[], uint32_t *type, bool delete, bool force)
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return -4;
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
                    return -4;
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
                        return -4;
                    }
                }
                if (bread(indirectb.blocks[(j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIR_ENTRY_PER_DIRECT_BLOCK],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    return -4;
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
                        return -4;
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
                        return -4;
                    }
                }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIR_ENTRY_PER_DIRECT_BLOCK],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    return -4;
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
                        return -4;
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
                        return -4;
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
                        return -4;
                    }
                }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIR_ENTRY_PER_DIRECT_BLOCK],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    return -4;
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
                    return -4;
                }
                if (!check_privilege(&inode, uid, 2))
                    return -2;
                if (!force && inode.type == 1 && inode.size > 2)
                    return -3;

                direntry_t de;
                if (_get_last_direntry(&dir_inode, &de))
                {
                    printf("获取最后一个目录项失败！\n");
                    return -4;
                }
                if (strcmp(filename, de.name))
                {
                    // 将最后一个目录项复制到当前位置。
                    db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK] = de;
                    if (bwrite(dbno, &db))
                    {
                        printf("修改数据块位图失败！\n");
                        return -4;
                    }
                }

                dir_inode.size--;
                if (dir_inode.size % DIR_ENTRY_PER_DIRECT_BLOCK == 0 && free_last_directb(&dir_inode))
                {
                    printf("释放目录块失败！\n");
                    return -4;
                }
                if (dir_inode.size % DIR_ENTRY_PER_INDIRECT_BLOCK == 0 && free_last_indirectb(&dir_inode))
                {
                    printf("释放中间目录块失败！\n");
                    return -4;
                }
                if (dir_inode.size % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0 && free_last_double_indirectb(&dir_inode))
                {
                    printf("释放二级中间目录块失败！\n");
                    return -4;
                }
                if (dir_inode.size % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0 && free_last_triple_indirectb(&dir_inode))
                {
                    printf("释放三级目录块失败！\n");
                    return -4;
                }
                if (iwrite(dir_ino, &dir_inode))
                {
                    printf("写入 inode 失败！\n");
                    return -4;
                }
            }
            return ino;
        }
    }
    return -1;
}

int create_file(uint32_t dir_ino, uint32_t uid, uint32_t type, const char filename[])
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return -5;
    }
    if (!check_privilege(&dir_inode, uid, 2))
        return -1;

    direntry_t de;
    de.type = type;
    strcpy(de.name, filename);
    de.ino = get_free_inode();
    switch (de.ino)
    {
    case -1:
        return -4;
        break;
    case -2:
        printf("获取空闲 inode 失败！\n");
        return -5;
    default:
        assert(de.ino < (uint32_t)-2);
        break;
    }

    int ret = _push_direntry(&dir_inode, &de);
    switch (ret)
    {
    case -1:
        return -2;
    case -2:
        return -3;
    case -3:
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    inode_t inode;
    inode.ino = de.ino;
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
    if (inode.type == 0)
        inode.privilege = 066;
    else
        inode.privilege = 077;
    if (inode.type == 1)
    {
        inode.direct_blocks[0] = get_free_data();
        switch (inode.direct_blocks[0])
        {
        case -1:
            return -3;
            break;
        case -2:
            printf("获取空闲数据块失败！\n");
            return -5;
        default:
            break;
        }
        dirblock_t new_db;
        if (bread(inode.direct_blocks[0], &new_db))
        {
            printf("读取目录块失败！\n");
            return -5;
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
            return -5;
        }
    }
    sbwrite(false);
    if (iwrite(de.ino, &inode))
    {
        printf("写入 inode 失败！\n");
        return -5;
    }
    if (iwrite(dir_ino, &dir_inode))
    {
        printf("写入 inode 失败！\n");
        return -5;
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
    if (!check_privilege(&inode, uid, 2))
        return -1;
    if (inode.type == 1)
        return -2;
    assert(inode.type == 0);
    assert(inode.bcnt <= DIRECT_DATA_BLOCK_CNT + INDIRECT_DATA_BLOCK_CNT + DOUBLE_INDIRECT_DATA_BLOCK_CNT +
                             TRIPLE_INDIRECT_DATA_BLOCK_CNT);

    if (inode.bcnt == 0 && unset_inode_bitmap(ino)) // 文件为空。
    {
        printf("修改 inode bitmap 失败！\n");
        return -3;
    }
    else if (inode.bcnt > 0)
    {
        indirectblock_t indirectb, double_indirectb, triple_indiectb;
        uint32_t indirectbno = 0, double_indirectbno = 0, triple_indirectbno = 0;
        for (uint32_t i = 0; i < inode.bcnt; i++)
        {
            if (i < DIRECT_DATA_BLOCK_CNT)
            {
                if (unset_data_bitmap(inode.direct_blocks[i]))
                {
                    printf("修改数据块位图失败！\n");
                    return -3;
                }
            }
            else if (i - INDIRECT_DATA_BLOCK_OFFSET < INDIRECT_DATA_BLOCK_CNT)
            {
                if ((i - INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK == 0)
                {
                    if (indirectbno && unset_data_bitmap(indirectbno))
                    {
                        printf("修改数据块位图失败！\n");
                        return -3;
                    }
                    if (bread(inode.indirect_blocks[(i - INDIRECT_DATA_BLOCK_OFFSET) / DATA_BLOCK_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return -3;
                    }
                    indirectbno =
                        inode.indirect_blocks[(i - INDIRECT_DATA_BLOCK_OFFSET) / DATA_BLOCK_PER_INDIRECT_BLOCK];
                }
                if (unset_data_bitmap(
                        indirectb.blocks[(i - INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK]))
                {
                    printf("修改数据块位图失败！\n");
                    return -3;
                }
            }
            else if ((i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) < DOUBLE_INDIRECT_DATA_BLOCK_CNT)
            {
                if ((i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK == 0)
                {
                    if (double_indirectbno && unset_data_bitmap(double_indirectbno))
                    {
                        printf("修改数据块位图失败！\n");
                        return -3;
                    }
                    if (bread(inode.double_indirect_blocks[(i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) /
                                                           DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取二级目录中间块失败！\n");
                        return -3;
                    }
                    double_indirectbno = inode.double_indirect_blocks[(i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) /
                                                                      DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK];
                }
                if ((i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK == 0)
                {
                    if (indirectbno && unset_data_bitmap(indirectbno))
                    {
                        printf("修改数据块位图失败！\n");
                        return -3;
                    }
                    if (bread(double_indirectb
                                  .blocks[(i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                          DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK / DATA_BLOCK_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return -3;
                    }
                    indirectbno =
                        double_indirectb.blocks[(i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                                DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK / DATA_BLOCK_PER_INDIRECT_BLOCK];
                }
                if (unset_data_bitmap(
                        indirectb.blocks[(i - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK]))
                {
                    printf("修改数据块位图失败！\n");
                    return -3;
                }
            }
            else
            {
                if ((i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK == 0)
                {
                    if (triple_indirectbno && unset_data_bitmap(triple_indirectbno))
                    {
                        printf("修改数据块位图失败！\n");
                        return -3;
                    }
                    if (bread(inode.triple_indirect_blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) /
                                                           DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK],
                              &triple_indiectb))
                    {
                        printf("读取三级目录中间块失败！\n");
                        return -3;
                    }
                    triple_indirectbno = inode.triple_indirect_blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) /
                                                                      DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK];
                }
                if ((i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK == 0)
                {
                    if (double_indirectbno && unset_data_bitmap(double_indirectbno))
                    {
                        printf("修改数据块位图失败！\n");
                        return -3;
                    }
                    if (bread(triple_indiectb
                                  .blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                          DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK / DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取二级目录中间块失败！\n");
                        return -3;
                    }
                    double_indirectbno =
                        triple_indiectb
                            .blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK /
                                    DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK];
                }
                if ((i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK == 0)
                {
                    if (indirectbno && unset_data_bitmap(indirectbno))
                    {
                        printf("修改数据块位图失败！\n");
                        return -3;
                    }
                    if (bread(double_indirectb
                                  .blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                          DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK / DATA_BLOCK_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        return -3;
                    }
                    indirectbno =
                        double_indirectb.blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                                DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK / DATA_BLOCK_PER_INDIRECT_BLOCK];
                }
                if (unset_data_bitmap(
                        indirectb.blocks[(i - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK]))
                {
                    printf("修改数据块位图失败！\n");
                    return -3;
                }
            }
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
            printf("修改 inode bitmap 失败！\n");
            return -3;
        }
    }

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
    if (!check_privilege(&inode, uid, 2))
        return -1;
    if (inode.type == 0)
        return -2;
    assert(inode.size >= 2);
    assert(inode.size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                             TRIPLE_INDIRECT_DIR_ENTRY_CNT);

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
                else
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

int read_file(uint32_t ino, uint32_t uid, uint32_t page, datablock_t *const db)
{
    inode_t inode;
    if (iread(ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -4;
    }
    if (!check_privilege(&inode, 0, 4))
        return -1;
    if (inode.type == 1)
        return -2;
    assert(inode.type == 0);
    assert(inode.bcnt <= DIRECT_DATA_BLOCK_CNT + INDIRECT_DATA_BLOCK_CNT + DOUBLE_INDIRECT_DATA_BLOCK_CNT +
                             TRIPLE_INDIRECT_DATA_BLOCK_CNT);

    if (page >= inode.bcnt)
        return -3;

    if (page < DIRECT_DATA_BLOCK_CNT)
    {
        if (bread(inode.direct_blocks[page], db))
        {
            printf("读取数据块失败！\n");
            return -4;
        }
    }
    else if (page - INDIRECT_DATA_BLOCK_OFFSET < INDIRECT_DATA_BLOCK_CNT)
    {
        indirectblock_t indirectb;
        if (bread(inode.indirect_blocks[(page - INDIRECT_DATA_BLOCK_OFFSET) / DATA_BLOCK_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -4;
        }
        if (bread(indirectb.blocks[(page - INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK], db))
        {
            printf("读取数据块失败！\n");
            return -4;
        }
    }
    else if ((page - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) < DOUBLE_INDIRECT_DATA_BLOCK_CNT)
    {
        indirectblock_t indirectb, double_indirectb;
        if (bread(inode.double_indirect_blocks[(page - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) /
                                               DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -4;
        }
        if (bread(double_indirectb.blocks[(page - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                          DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK / DATA_BLOCK_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -4;
        }
        if (bread(indirectb.blocks[(page - DOUBLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK], db))
        {
            printf("读取数据块失败！\n");
            return -4;
        }
    }
    else
    {
        indirectblock_t indirectb, double_indirectb, triple_indiectb;
        if (bread(inode.triple_indirect_blocks[(page - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) /
                                               DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK],
                  &triple_indiectb))
        {
            printf("读取三级目录中间块失败！\n");
            return -4;
        }
        if (bread(triple_indiectb.blocks[(page - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                         DATA_BLOCK_PER_TRIPLE_INDIRECT_BLOCK / DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK],
                  &double_indirectb))
        {
            printf("读取二级目录中间块失败！\n");
            return -4;
        }
        if (bread(double_indirectb.blocks[(page - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) %
                                          DATA_BLOCK_PER_DOUBLE_INDIRECT_BLOCK / DATA_BLOCK_PER_INDIRECT_BLOCK],
                  &indirectb))
        {
            printf("读取目录中间块失败！\n");
            return -4;
        }
        if (bread(indirectb.blocks[(page - TRIPLE_INDIRECT_DATA_BLOCK_OFFSET) % DATA_BLOCK_PER_INDIRECT_BLOCK], db))
        {
            printf("读取数据块失败！\n");
            return -4;
        }
    }
    return 0;
}

int copy_file(uint32_t dir_ino, uint32_t ino, uint32_t uid, const char filename[])
{
    inode_t dir_inode, inode;
    if (iread(dir_ino, &dir_inode) || iread(ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -5;
    }
    assert(dir_inode.type == 1 && inode.type == 0);
    if (!check_privilege(&dir_inode, uid, 2) || !check_privilege(&inode, uid, 4))
        return -1;

    assert(inode.bcnt <= DIRECT_DATA_BLOCK_CNT + INDIRECT_DATA_BLOCK_CNT + DOUBLE_INDIRECT_DATA_BLOCK_CNT +
                             TRIPLE_INDIRECT_DATA_BLOCK_CNT);
    assert(dir_inode.size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                 TRIPLE_INDIRECT_DIR_ENTRY_CNT);

    if (dir_inode.size ==
        DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT + TRIPLE_INDIRECT_DIR_ENTRY_CNT)
        return -2;

    uint32_t new_ino = get_free_inode();
    switch (new_ino)
    {
    case -1:
        return -4;
    case -2:
        printf("获取空闲 inode 失败！\n");
        return -5;
    default:
        assert(new_ino < (uint32_t)-2);
        break;
    }
    direntry_t de;
    de.ino = new_ino;
    de.type = 0;
    strcpy(de.name, filename);
    int ret = _push_direntry(&dir_inode, &de);
    switch (ret)
    {
    case -1:
        return -2;
    case -2:
        return -3;
    case -3:
        return -5;
    default:
        assert(ret == 0);
        break;
    }

    inode_t new_inode;
    new_inode = inode;
    new_inode.ino = new_ino;
    new_inode.size = 0;
    new_inode.bcnt = 0;
    new_inode.ctime = (uint32_t)time(NULL);
    new_inode.wtime = (uint32_t)time(NULL);

    // 拷贝原 inode 中的数据。
    datablock_t db;
    for (uint32_t i = 0; i < inode.bcnt; i++)
    {
        if (_read_file(&inode, i, &db))
        {
            printf("读取源文件失败！\n");
            return -5;
        }
        int ret = _append_block(&new_inode, &db);
        switch (ret)
        {
        case -1:
            return -3;
        case -2:
            printf("写入目标文件失败！\n");
            return -5;
        default:
            assert(ret == 0);
            break;
        }
    }

    if (iwrite(dir_ino, &dir_inode) || iwrite(ino, &inode) || iwrite(new_ino, &new_inode))
    {
        printf("写入 inode 失败！\n");
        return -5;
    }

    return 0;
}

int copy_from_host(uint32_t dir_ino, uint32_t uid, const char host_filename[], const char filename[])
{
    inode_t dir_inode;
    if (iread(dir_ino, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        return -6;
    }
    if (!check_privilege(&dir_inode, uid, 2))
        return -1;
    assert(dir_inode.size <= DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT +
                                 TRIPLE_INDIRECT_DIR_ENTRY_CNT);
    if (dir_inode.size ==
        DIRECT_DIR_ENTRY_CNT + INDIRECT_DIR_ENTRY_CNT + DOUBLE_INDIRECT_DIR_ENTRY_CNT + TRIPLE_INDIRECT_DIR_ENTRY_CNT)
        return -2;

    uint32_t ino = get_free_inode();
    switch (ino)
    {
    case -1:
        return -4;
    case -2:
        printf("获取空闲 inode 失败！\n");
        return -6;
    default:
        break;
    }
    inode_t inode;
    inode.ino = ino;
    inode.type = 0;
    inode.ctime = (uint32_t)time(NULL);
    inode.wtime = (uint32_t)time(NULL);
    inode.uid = uid;
    inode.privilege = 066;
    inode.size = 0;
    inode.bcnt = 0;

    direntry_t de;
    de.ino = ino;
    de.type = 0;
    strcpy(de.name, filename);
    int ret = _push_direntry(&dir_inode, &de);
    switch (ret)
    {
    case -1:
        return -2;
    case -2:
        return -3;
    case -3:
        printf("追加目录项失败！\n");
        return -6;
    default:
        assert(ret == 0);
        break;
    }

    FILE *file = fopen(host_filename, "r");
    if (!file)
        return -5;

    datablock_t db;
    memset(&db, 0, BLOCK_SIZE);
    int read;
    do
    {
        read = fread(&db, 1, BLOCK_SIZE, file);
        if (read == 0)
            break;
        int ret = _append_block(&inode, &db);
        switch (ret)
        {
        case -1:
            return -3;
        case -2:
            printf("写入目标文件失败！\n");
            return -6;
        default:
            assert(ret == 0);
            break;
        }
    } while (read == BLOCK_SIZE);

    if (iwrite(dir_ino, &dir_inode) || iwrite(ino, &inode))
    {
        printf("写入 inode 失败！\n");
        return -6;
    }

    return 0;
}

int copy_to_host(uint32_t ino, uint32_t uid, char host_dirname[], const char filename[])
{
    inode_t inode;
    if (iread(ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -3;
    }
    if (!check_privilege(&inode, uid, 4))
        return -1;

    FILE *file = fopen(strcat(host_dirname, filename), "w");
    if (!file)
        return -2;
    datablock_t db;
    for (uint32_t i = 0; i < inode.bcnt; i++)
    {
        _read_file(&inode, i, &db);
        fwrite(&db, 1, BLOCK_SIZE, file);
    }
    fclose(file);

    return 0;
}

int change_privilege(uint32_t ino, uint32_t uid, uint32_t privilege)
{
    inode_t inode;
    if (iread(ino, &inode))
    {
        printf("读取 inode 失败！\n");
        return -2;
    }
    if (uid != 0 && inode.uid != uid)
        return -1;
    inode.privilege = privilege;
    if (iwrite(ino, &inode))
    {
        printf("写入 inode 失败！\n");
        return -2;
    }
    return 0;
}