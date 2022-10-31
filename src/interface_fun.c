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
    i += 2 + num2width(sb.ucnt);
    strcpy(spec_shm + i, "块大小：");
    i += 12;
    sprintf(spec_shm + i, "%u\r\n", sb.bsize);
    i += 2 + num2width(sb.bsize);
    strcpy(spec_shm + i, "块数量：");
    i += 12;
    sprintf(spec_shm + i, "%u\r\n", sb.bcnt);
    i += 2 + num2width(sb.bcnt);
    strcpy(spec_shm + i, "inode 数量：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.icnt);
    i += 2 + num2width(sb.icnt);
    strcpy(spec_shm + i, "空闲 inode 数量：");
    i += 22;
    sprintf(spec_shm + i, "%u\r\n", sb.free_icnt);
    i += 2 + num2width(sb.free_icnt);
    strcpy(spec_shm + i, "inode 大小：");
    i += 15;
    sprintf(spec_shm + i, "%u\r\n", sb.isize);
    i += 2 + num2width(sb.isize);
    strcpy(spec_shm + i, "inode 块数量：");
    i += 18;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bcnt);
    i += 2 + num2width(sb.inode_bcnt);
    strcpy(spec_shm + i, "数据块数量：");
    i += 18;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bcnt);
    i += 2 + num2width(sb.data_bcnt);
    strcpy(spec_shm + i, "空闲数据块数量：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.free_data_bcnt);
    i += 2 + num2width(sb.free_data_bcnt);
    strcpy(spec_shm + i, "inode 位图块数量：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bitmap_bcnt);
    i += 2 + num2width(sb.inode_bitmap_bcnt);
    strcpy(spec_shm + i, "数据块位图块数量：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bitmap_bcnt);
    i += 2 + num2width(sb.data_bitmap_bcnt);
    strcpy(spec_shm + i, "inode table 起始块号：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_table_start);
    i += 2 + num2width(sb.inode_table_start);
    strcpy(spec_shm + i, "inode 位图起始块号：");
    i += 27;
    sprintf(spec_shm + i, "%u\r\n", sb.inode_bitmap_start);
    i += 2 + num2width(sb.inode_bitmap_start);
    strcpy(spec_shm + i, "数据块位图起始块号：");
    i += 30;
    sprintf(spec_shm + i, "%u\r\n", sb.data_bitmap_start);
    i += 2 + num2width(sb.data_bitmap_start);
    strcpy(spec_shm + i, "数据块起始块号：");
    i += 24;
    sprintf(spec_shm + i, "%u\r\n", sb.data_start);
    i += 2 + num2width(sb.data_start);

    return i;
}

uint16_t cd(msg_t *msg)
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
        if (r < cmd_dir_len - 1)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "cd: 目录不存在！\r\n");
                return 24;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search_file(working_dir, filename, &type);
            if (working_dir == 0xfffffffe)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == 0xffffffff || type != 1)
            {
                strcpy(spec_shm, "cd: 目录不存在！\r\n");
                return 24;
            }
        }
        else // 说明已经是最后一个目录。
        {
            if (cmd_dir[r] == '/')
                cmd_dir[r--] = 0;
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "cd: 目录不存在！\r\n");
                return 24;
            }
            uint32_t ino = search_file(working_dir, cmd_dir + l, NULL);
            if (ino == 0xfffffffe)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (ino == 0xffffffff)
            {
                strcpy(spec_shm, "cd: 目录不存在！\r\n");
                return 24;
            }
            else
                working_dirs[msg->uid] = ino;

            break;
        }
        r++;
        l = r;
    }
    return 0; // 若未出错， mkdir 不会有字符进入缓冲区。
}

uint16_t ls(uint32_t uid, bool detial)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[uid];
    inode_t dir_inode;
    if (iread(working_dirs[uid], &dir_inode))
    {
        printf("读取 inode 失败！\n");
        strcpy(spec_shm + i, "ERROR");
        return 5;
    }
    assert(dir_inode.type == 1);
    indirectblock_t indirectb, double_indirectb, triple_indiectb;
    dirblock_t db;

    for (uint32_t j = 0; j < dir_inode.size; j++)
    {
        if (j % DIR_ENTRY_PER_DIRECT_BLOCK == 0)
        {
            if (j < DIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if (bread(dir_inode.direct_blocks[j / DIR_ENTRY_PER_DIRECT_BLOCK], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if (j - INDIRECT_BLOCK_OFFSET < INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.indirect_blocks[(j - INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                              &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - INDIRECT_BLOCK_OFFSET) / DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) < DOUBLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.double_indirect_blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) /
                                                               DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - DOUBLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_BLOCK_OFFSET) / DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
            else if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) < TRIPLE_INDIRECT_BLOCK_DIR_ENTRY_CNT)
            {
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK == 0)
                    if (bread(dir_inode.triple_indirect_blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) /
                                                               DIR_ENTRY_PER_TRIPLE_INDIRECT_BLOCK],
                              &triple_indiectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK == 0)
                    if (bread(triple_indiectb
                                  .blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_DOUBLE_INDIRECT_BLOCK],
                              &double_indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if ((j - TRIPLE_INDIRECT_BLOCK_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK == 0)
                    if (bread(
                            double_indirectb.blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIR_ENTRY_PER_INDIRECT_BLOCK],
                            &indirectb))
                    {
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_BLOCK_OFFSET) / DIRECT_BLOCK_DIR_ENTRY_CNT], &db))
                {
                    printf("读取目录块失败！\n");
                    strcpy(spec_shm, "ERROR");
                    return i > 5 ? i : 5;
                }
            }
        }
        if (detial)
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
                i += num2width(inode.size) + 1;
            }
            else
            {
                sprintf(spec_shm + i, "%u\t", inode.bcnt * BLOCK_SIZE);
                i += num2width(inode.bcnt * BLOCK_SIZE) + 1;
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
    strcpy(spec_shm + i, "\r\n");
    i += 2;

    return i;
}

uint16_t md(msg_t *msg)
{
    void *spec_shm = spec_shms[msg->uid];

    char cmd_dir[CMD_LEN - 3];
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "mkdir", 5))
        strcpy(cmd_dir, msg->cmd + 6);
    else if (!strncmp(msg->cmd, "md", 2))
        strcpy(cmd_dir, msg->cmd + 3);
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
        if (r < cmd_dir_len - 1)
        {
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "mkdir: 目录不存在！\r\n");
                return 27;
            }
            char filename[FILE_NAME_LEN];
            memset(filename, 0, FILE_NAME_LEN);
            strncpy(filename, cmd_dir + l, r - l);
            uint32_t type;
            working_dir = search_file(working_dir, filename, &type);
            if (working_dir == 0xfffffffe)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (working_dir == 0xffffffff || type != 1)
            {
                strcpy(spec_shm, "mkdir: 目录不存在！\r\n");
                return 27;
            }
        }
        else // 说明已经是最后一个目录。
        {
            if (cmd_dir[r] == '/')
                cmd_dir[r--] = 0;
            if (r - l > FILE_NAME_LEN)
            {
                strcpy(spec_shm, "mkdir: 目录名过长！\r\n");
                return 27;
            }
            uint32_t ino = search_file(working_dir, cmd_dir + l, NULL);
            if (ino == 0xfffffffe)
            {
                strcpy(spec_shm, "ERROR");
                return 5;
            }
            else if (ino == 0xffffffff)
            {
                int ret = create_file(working_dir, msg->uid, 1, cmd_dir + l);
                if (ret == -1)
                {
                    strcpy(spec_shm, "mkdir: 目录容量不足！\r\n");
                    return 30;
                }
                else if (ret == -2)
                {
                    strcpy(spec_shm, "ERROR");
                    return 5;
                }
                assert(ret == 0);
            }
            else
            {
                strcpy(spec_shm, "mkdir: 同名目录或文件已存在！\r\n");
                return 42;
            }
            break;
        }
        r++;
        l = r;
    }
    return 0; // 若未出错， mkdir 不会有字符进入缓冲区。
}

uint16_t unknown(uint32_t uid)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[uid];
    strcpy(spec_shm + i, "无法识别该命令！\r\n");
    i += 26;
    return i;
}