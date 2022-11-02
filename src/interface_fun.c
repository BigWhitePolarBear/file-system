#include "interface_fun.h"
#include "assert.h"
#include "cmd.h"
#include "device.h"
#include "internal_fun.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

int login(uint32_t uid, const char pwd[])
{
    return !strncmp(sb.users[uid].pwd, pwd, PWD_LEN);
}

// 以下函数的实现中，往往会看到一个 uint16_t 类型的 i 变量，
// 这个变量是返回值，代表在共享内存中写入到的位置，方便外部函数清理共享内存。

uint16_t info(uint32_t uid)
{
    struct tm lt;
    time_t t;

    uint16_t i = 0;
    void *spec_shm = spec_shms[uid];
    float storage_size = sb.bsize * sb.data_bcnt / 1024 / 1024;
    float free_storage_size = sb.bsize * sb.free_data_bcnt / 1024 / 1024;
    strcpy(spec_shm + i, "总存储空间：");
    i += 18;
    sprintf(spec_shm + i, "%.2fMB\r\n", storage_size);
    i += float2width(storage_size) + 7;
    strcpy(spec_shm + i, "可用存储空间：");
    i += 21;
    sprintf(spec_shm + i, "%.2fMB\r\n", free_storage_size);
    i += float2width(storage_size) + 7;
    strcpy(spec_shm + i, "系统状态：");
    i += 15;
    if (sb.status)
        strcpy(spec_shm + i, "异常\r\n");
    else
        strcpy(spec_shm + i, "正常\r\n");
    i += 8;
    strcpy(spec_shm + i, "格式化时间：");
    i += 18;
    t = sb.fmt_time;
    localtime_r(&t, &lt);
    sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\r\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
            lt.tm_min, lt.tm_sec);
    i += 21;
    strcpy(spec_shm + i, "最后修改时间：");
    i += 21;
    t = sb.fmt_time;
    localtime_r(&t, &lt);
    sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\r\n", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
            lt.tm_min, lt.tm_sec);
    i += 21;
    strcpy(spec_shm + i, "用户数量：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.ucnt);
    i += 2 + uint2width(sb.ucnt);
    strcpy(spec_shm + i, "块大小：");
    i += 12;
    sprintf(spec_shm + i, "%u\r\n", sb.bsize);
    i += 2 + uint2width(sb.bsize);
    strcpy(spec_shm + i, "块数量：");
    i += 12;
    sprintf(spec_shm + i, "%u\r\n", sb.bcnt);
    i += 2 + uint2width(sb.bcnt);
    strcpy(spec_shm + i, "inode 数量：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.icnt);
    i += 2 + uint2width(sb.icnt);
    strcpy(spec_shm + i, "空闲 inode 数量：");
    i += 22;
    sprintf(spec_shm + i, "%u\r\n", sb.free_icnt);
    i += 2 + uint2width(sb.free_icnt);
    strcpy(spec_shm + i, "inode 大小：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.isize);
    i += 2 + uint2width(sb.isize);
    strcpy(spec_shm + i, "inode 块数量：");
    i += 18;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bcnt);
    i += 2 + uint2width(sb.inode_bcnt);
    strcpy(spec_shm + i, "数据块数量：");
    i += 18;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bcnt);
    i += 2 + uint2width(sb.data_bcnt);
    strcpy(spec_shm + i, "空闲数据块数量：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.free_data_bcnt);
    i += 2 + uint2width(sb.free_data_bcnt);
    strcpy(spec_shm + i, "inode 位图块数量：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bitmap_bcnt);
    i += 2 + uint2width(sb.inode_bitmap_bcnt);
    strcpy(spec_shm + i, "数据块位图块数量：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bitmap_bcnt);
    i += 2 + uint2width(sb.data_bitmap_bcnt);
    strcpy(spec_shm + i, "inode table 起始块号：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_table_start);
    i += 2 + uint2width(sb.inode_table_start);
    strcpy(spec_shm + i, "inode 位图起始块号：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bitmap_start);
    i += 2 + uint2width(sb.inode_bitmap_start);
    strcpy(spec_shm + i, "数据块位图起始块号：");
    i += 30;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bitmap_start);
    i += 2 + uint2width(sb.data_bitmap_start);
    strcpy(spec_shm + i, "数据块起始块号：");
    i += 24;
    sprintf(spec_shm + i, "%u", sb.data_start);
    i += uint2width(sb.data_start);

    return i;
}

uint16_t cd(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 3];
    strcpy(cmd_dir, msg->cmd + 3);
    uint8_t cmd_dir_len = strlen(cmd_dir);

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir[0] == '/')
    {
        if (cmd_dir_len == 1)
        {
            working_dirs[msg->uid] = 0;
            return 0;
        }
        working_dir = 0;
        l = r = 1;
    }
    if (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;
    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;
        if (r < cmd_dir_len)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "cd: 目录不存在！");
                return 22;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search(working_dir, msg->uid, filename, &type, false, false);
            if (working_dir == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == -1 || type != 1)
            {
                strcpy(spec_shm, "cd: 目录不存在！");
                return 22;
            }
        }
        else // 说明已经是最后一个目录。
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "cd: 目录不存在！");
                return 22;
            }
            uint32_t ino = search(working_dir, msg->uid, cmd_dir + l, NULL, false, false);
            if (ino == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (ino == -1)
            {
                strcpy(spec_shm, "cd: 目录不存在！");
                return 22;
            }
            else
            {
                assert(ino < (uint32_t)-4);

                inode_t inode;
                if (iread(ino, &inode))
                {
                    printf("读取 inode 失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return 5;
                }
                if (!check_privilege(&inode, msg->uid, 1))
                {
                    strcpy(spec_shm, "cd: 权限不足！");
                    return 19;
                }

                working_dirs[msg->uid] = ino;
            }

            break;
        }
        r++;
        l = r;
    }
    return 0; // 若未出错， cd 不会有字符进入缓冲区。
}

uint16_t ls(const msg_t *const msg)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 3];
    bool detail;
    if (!strncmp(msg->cmd, "ls", 2))
    {
        if ((detail = !strncmp(msg->cmd + 3, "-l", 2)))
            strcpy(cmd_dir, msg->cmd + 6);
        else
            strcpy(cmd_dir, msg->cmd + 3);
    }
    else if (!strncmp(msg->cmd, "dir", 3))
    {
        if ((detail = !strncmp(msg->cmd + 4, "-s", 2)))
            strcpy(cmd_dir, msg->cmd + 7);
        else
            strcpy(cmd_dir, msg->cmd + 4);
    }
    uint8_t cmd_dir_len = strlen(cmd_dir);
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir_len == 0)
        goto GetInfo;

    uint8_t l = 0, r = 0;
    if (cmd_dir[0] == '/')
    {
        if (cmd_dir_len == 1)
        {
            working_dirs[msg->uid] = 0;
            return 0;
        }
        working_dir = 0;
        l = r = 1;
    }
    if (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;

        if (r - l > FILE_NAME_LEN)
        {
            strcpy(spec_shm, "ls: 目录不存在！");
            return 22;
        }
        char filename[FILE_NAME_LEN];
        memset(filename, 0, FILE_NAME_LEN);
        strncpy(filename, cmd_dir + l, r - l);
        uint32_t type;
        working_dir = search(working_dir, msg->uid, filename, &type, false, false);
        if (working_dir == -4)
        {
            strcpy(spec_shm, "ERROR");
            return 5;
        }
        else if (working_dir == -1 || type != 1)
        {
            strcpy(spec_shm, "ls: 目录不存在！");
            return 22;
        }
        assert(working_dir < (uint32_t)-4);

        if (r == cmd_dir_len)
            break;
        r++;
        l = r;
    }
GetInfo:
    inode_t dir_inode;
    if (iread(working_dir, &dir_inode))
    {
        printf("读取 inode 失败！\n");
        strcpy(spec_shm + i, "ERROR");
        return 5;
    }
    assert(dir_inode.type == 1);
    if (!check_privilege(&dir_inode, msg->uid, 4))
    {
        strcpy(spec_shm + i, "ls: 权限不足！");
        return 19;
    }

    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;

    for (uint32_t j = 0; j < dir_inode.size; j++)
    {
        if (j % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if (j < DIRECT_DIR_ENTRY_CNT)
            {
                if (bread(dir_inode.direct_blocks[j / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if (j - INDIRECT_DIR_OFFSET < INDIRECT_DIR_ENTRY_CNT)
            {
                if ((j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.indirect_blocks[(j - INDIRECT_DIR_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb
                              .blocks[(j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIRECT_DIR_ENTRY_CNT],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if ((j - DOUBLE_INDIRECT_DIR_OFFSET) < DOUBLE_INDIRECT_DIR_ENTRY_CNT)
            {
                if ((j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.double_indirect_blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) %
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIRECT_DIR_ENTRY_CNT],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if ((j - TRIPLE_INDIRECT_DIR_OFFSET) < TRIPLE_INDIRECT_DIR_ENTRY_CNT)
            {
                if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.triple_indirect_blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) /
                                                               DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                              &triple_indiectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(triple_indiectb
                                  .blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK /
                                          DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) %
                                                    DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIRECT_DIR_ENTRY_CNT],
                          &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
        }
        if (detail)
        {
            uint32_t ino = db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].ino;
            inode_t inode;
            if (iread(ino, &inode))
            {
                printf("读取 inode 失败！\n");
                strcpy(spec_shm, "ERROR");
                return i > 5 ? i : 5;
            }
            // 权限。
            if (inode.type == 1)
                strcpy(spec_shm + i++, "d");
            else
                strcpy(spec_shm + i++, "-");
            if (inode.privilege & 040)
                strcpy(spec_shm + i++, "r");
            else
                strcpy(spec_shm + i++, "-");
            if (inode.privilege & 020)
                strcpy(spec_shm + i++, "w");
            else
                strcpy(spec_shm + i++, "-");
            if (inode.privilege & 010)
                strcpy(spec_shm + i++, "x");
            else
                strcpy(spec_shm + i++, "-");
            if (inode.privilege & 004)
                strcpy(spec_shm + i++, "r");
            else
                strcpy(spec_shm + i++, "-");
            if (inode.privilege & 002)
                strcpy(spec_shm + i++, "w");
            else
                strcpy(spec_shm + i++, "-");
            if (inode.privilege & 001)
                strcpy(spec_shm + i++, "x");
            else
                strcpy(spec_shm + i++, "-");
            strcpy(spec_shm + i++, "\t");

            // 拥有者。
            sprintf(spec_shm + i, "%2u\t", inode.uid);
            i += 3;

            // 文件大小或目录元数据占用空间。
            if (inode.type == 0)
            {
                sprintf(spec_shm + i, "%u\t", inode.size);
                i += uint2width(inode.size) + 1;
            }
            else
            {
                assert(inode.type == 1);
                sprintf(spec_shm + i, "%u\t", inode.bcnt * BLOCK_SIZE);
                i += uint2width(inode.bcnt * BLOCK_SIZE) + 1;
            }

            struct tm lt;
            time_t t;
            // 创建时间。
            t = inode.ctime;
            localtime_r(&t, &lt);
            sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\t", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                    lt.tm_hour, lt.tm_min, lt.tm_sec);
            i += 20;

            // 创建时间。
            t = inode.wtime;
            localtime_r(&t, &lt);
            sprintf(spec_shm + i, "%04d-%02d-%02d %02d:%02d:%02d\t", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                    lt.tm_hour, lt.tm_min, lt.tm_sec);
            i += 20;

            // 名称
            if (inode.type == 1)
            {
                sprintf(spec_shm + i, "\033[0;34m%s/\033[0m", db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name);
                i += 12; // 改变颜色的控制字符以及 '/' 。
            }
            else
                strcpy(spec_shm + i, db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name);
            i += strlen(db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name);

            if (j + 1 < dir_inode.size)
            {
                strcpy(spec_shm + i, "\r\n");
                i += 2;
            }
        }
        else
        {
            if (db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].type == 1)
            {
                sprintf(spec_shm + i, "\033[0;34m%s/\033[0m", db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name);
                i += 12; // 改变颜色的控制字符以及 '/' 。
            }
            else
                strcpy(spec_shm + i, db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name);
            i += strlen(db.direntries[j % DIR_ENTRY_PER_DIRECT_BLOCK].name);
            strcpy(spec_shm + i++, "\t");

            if ((j + 1) % 5 == 0)
            {
                strcpy(spec_shm + i, "\r\n");
                i += 2;
            }
        }
    }
    return i;
}

uint16_t md(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 3];
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "md", 2))
        strcpy(cmd_dir, msg->cmd + 3);
    else if (!strncmp(msg->cmd, "mkdir", 5))
        strcpy(cmd_dir, msg->cmd + 6);
    uint8_t cmd_dir_len = strlen(cmd_dir);

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }
    if (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;
        if (r < cmd_dir_len)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "mkdir: 目录不存在！");
                return 25;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search(working_dir, msg->uid, filename, &type, false, false);
            if (working_dir == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == -1 || type != 1)
            {
                strcpy(spec_shm, "mkdir: 目录不存在！");
                return 25;
            }
        }
        else // 说明已经是最后一个目录。
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "mkdir: 目录名过长！");
                return 25;
            }
            uint32_t ino = search(working_dir, msg->uid, cmd_dir + l, NULL, false, false);
            if (ino == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (ino == -1)
            {
                int ret = create_file(working_dir, msg->uid, 1, cmd_dir + l);
                switch (ret)
                {
                case -1:
                    strcpy(spec_shm, "mkdir: 权限不足！");
                    return 22;
                case -2:
                    strcpy(spec_shm, "mkdir: 目录容量不足！");
                    return 28;
                case -3:
                    strcpy(spec_shm, "ERROR");
                    return 5;
                default:
                    assert(ret == 0);
                    break;
                }
            }
            else
            {
                assert(ino < (uint32_t)-4);
                strcpy(spec_shm, "mkdir: 同名目录或文件已存在！");
                return 40;
            }
            break;
        }
        r++;
        l = r;
    }
    return 0; // 若未出错， mkdir 不会有字符进入缓冲区。
}

uint16_t rd(const msg_t *const msg)
{
    bool force = false;
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 3];
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "rd", 2))
    {
        if (!strncmp(msg->cmd + 3, "-f", 2))
        {
            force = true;
            strcpy(cmd_dir, msg->cmd + 6);
        }
        else
            strcpy(cmd_dir, msg->cmd + 3);
    }
    else if (!strncmp(msg->cmd, "rmdir", 5))
    {
        if (!strncmp(msg->cmd + 6, "-f", 2))
        {
            force = true;
            strcpy(cmd_dir, msg->cmd + 9);
        }
        else
            strcpy(cmd_dir, msg->cmd + 6);
    }
    uint8_t cmd_dir_len = strlen(cmd_dir);

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }
    if (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;
        if (r < cmd_dir_len)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "rmdir: 目录不存在！");
                return 25;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search(working_dir, msg->uid, filename, &type, false, false);
            if (working_dir == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == -1 || type != 1)
            {
                strcpy(spec_shm, "rmdir: 目录不存在！");
                return 25;
            }
            assert(working_dir < (uint32_t)-4);
        }
        else // 说明已经是最后一个目录。
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "rmdir: 目录不存在！");
                return 25;
            }
            uint32_t ino = search(working_dir, msg->uid, cmd_dir + l, NULL, true, force);
            switch (ino)
            {
            case -1:
                strcpy(spec_shm, "rmdir: 目录不存在！");
                return 25;
            case -2:
                strcpy(spec_shm, "rmdir: 权限不足！");
                return 22;
            case -3:
                strcpy(spec_shm, "rmdir: 目录不为空，可添加 -f 参数强制删除！");
                return 59;
            case -4:
                strcpy(spec_shm, "ERROR");
                return 5;
            default:
                int ret = remove_dir(ino, msg->uid);
                switch (ret)
                {
                case -1:
                    strcpy(spec_shm, "rmdir: 权限不足！");
                    return 22;
                case -2:
                    strcpy(spec_shm, "rmdir: 目标为文件！");
                    return 25;
                case -3:
                    strcpy(spec_shm, "ERROR");
                    return 5;
                default:
                    assert(ret == 0);
                    break;
                }
            }
            break;
        }
        r++;
        l = r;
    }

    return 0; // 若未出错， rmdir 不会有字符进入缓冲区。
}

uint16_t newfile(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 8];
    strcpy(cmd_dir, msg->cmd + 8);
    uint8_t cmd_dir_len = strlen(cmd_dir);

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir[0] == '/')
    {
        if (cmd_dir_len == 1)
        {
            working_dirs[msg->uid] = 0;
            return 0;
        }
        working_dir = 0;
        l = r = 1;
    }
    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;
        if (r < cmd_dir_len)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "newfile: 目录不存在！");
                return 27;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search(working_dir, msg->uid, filename, &type, false, false);
            if (working_dir == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == -1 || type != 1)
            {
                strcpy(spec_shm, "newfile: 目录不存在！");
                return 27;
            }
        }
        else // 说明已经是文件名。
        {
            if (r == l)
            {
                strcpy(spec_shm, "newfile:文件名不能为空！");
                return 32;
            }
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "newfile:文件名过长！");
                return 26;
            }
            uint32_t ino = search(working_dir, msg->uid, cmd_dir + l, NULL, false, false);
            if (ino == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (ino < (uint32_t)-4)
            {
                strcpy(spec_shm, "newfile: 文件已存在！");
                return 27;
            }
            else
            {
                assert(ino == (uint32_t)-1);
                int ret = create_file(working_dir, msg->uid, 0, cmd_dir + l);
                switch (ret)
                {
                case -1:
                    strcpy(spec_shm, "newfile: 权限不足！");
                    return 24;
                case -2:
                    strcpy(spec_shm, "newfile: 目录容量不足！");
                    return 30;
                case -3:
                    strcpy(spec_shm, "ERROR");
                    return 5;
                default:
                    assert(ret == 0);
                    break;
                }
            }
            break;
        }
        r++;
        l = r;
    }
    return 0; // 若未出错， rmdir 不会有字符进入缓冲区。
}

uint16_t rm(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 3];
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "rm", 2))
        strcpy(cmd_dir, msg->cmd + 3);
    else if (!strncmp(msg->cmd, "del", 3))
        strcpy(cmd_dir, msg->cmd + 4);

    uint8_t cmd_dir_len = strlen(cmd_dir);

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }
    if (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;
        if (r < cmd_dir_len)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "rm: 文件不存在！");
                return 22;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search(working_dir, msg->uid, filename, &type, false, false);
            if (working_dir == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == -1 || type != 1)
            {
                strcpy(spec_shm, "rm: 文件不存在！");
                return 22;
            }
        }
        else // 说明已经是最后一个目录。
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "rm: 文件不存在！");
                return 22;
            }
            uint32_t ino = search(working_dir, msg->uid, cmd_dir + l, NULL, true, false);
            switch (ino)
            {
            case -1:
                strcpy(spec_shm, "rm: 文件不存在！");
                return 22;
            case -4:
                strcpy(spec_shm, "ERROR");
                return 5;
            default:
                assert(ino < (uint32_t)-4);
                int ret = remove_file(ino, msg->uid);
                switch (ret)
                {
                case -1:
                    strcpy(spec_shm, "rm: 权限不足！");
                    return 19;
                case -2:
                    strcpy(spec_shm, "rm: 目标为目录！");
                    return 22;
                case -3:
                    strcpy(spec_shm, "ERROR");
                    return 5;
                default:
                    assert(ret == 0);
                    break;
                }
            }
            break;
        }
        r++;
        l = r;
    }

    return 0; // 若未出错， rm 不会有字符进入缓冲区。
}

uint16_t cat(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->uid];

    uint32_t page = 0;
    char cmd_dir[CMD_LEN - 3];
    if (!strncmp(msg->cmd + 4, "-p", 2))
    {
        uint8_t i = 6;
        while (true)
        {
            char c = *(msg->cmd + (++i));
            if (!('0' <= c && c <= '9'))
                break;
            page = page * 10 + c - '0';
        }
        strcpy(cmd_dir, msg->cmd + i);
    }
    else
        strcpy(cmd_dir, msg->cmd + 4);

    uint8_t cmd_dir_len = strlen(cmd_dir);

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->uid];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }
    if (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;
        if (r < cmd_dir_len)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "cat: 文件不存在！");
                return 23;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search(working_dir, msg->uid, filename, &type, false, false);
            if (working_dir == -4)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == -1 || type != 1)
            {
                strcpy(spec_shm, "cat: 文件不存在！");
                return 23;
            }
        }
        else // 说明已经是最后一个目录。
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "cat: 文件不存在！");
                return 23;
            }
            uint32_t ino = search(working_dir, msg->uid, cmd_dir + l, NULL, false, false);
            switch (ino)
            {
            case -1:
                strcpy(spec_shm, "cat: 文件不存在！");
                return 23;
            case -4:
                strcpy(spec_shm, "ERROR");
                return 5;
            default:
                assert(ino < (uint32_t)-4);
                int ret = read_file(ino, msg->uid, page);
                switch (ret)
                {
                case -1:
                    strcpy(spec_shm, "cat: 权限不足！");
                    return 20;
                case -2:
                    strcpy(spec_shm, "cat: 目标为目录！");
                    return 23;
                case -3:
                    strcpy(spec_shm, "cat: 访问内容超出文件大小！");
                    return 38;
                case -4:
                    strcpy(spec_shm, "ERROR");
                    return 5;
                default:
                    assert(ret == 0);
                    break;
                }
            }
            break;
        }
        r++;
        l = r;
    }
    return BLOCK_SIZE;
}

uint16_t unknown(uint32_t uid)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[uid];
    strcpy(spec_shm + i, "无法识别该命令！");
    i += 24;
    return i;
}