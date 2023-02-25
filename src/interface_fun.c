#include "interface_fun.h"
#include "assert.h"
#include "base_fun.h"
#include "cmd.h"
#include "device.h"
#include "internal_fun.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

int login(uint32_t uid, const char pwd[])
{
    // 视为未注册。
    if (strlen(sb.users[uid].pwd) == 0)
    {
        strncpy(sb.users[uid].pwd, pwd, PWD_LEN);
        sbwrite(false);
        return 1;
    }
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

    pthread_rwlock_rdlock(sb_lock);

    float storage_size = sb.bsize * sb.data_bcnt / 1024. / 1024.;
    float free_storage_size = sb.bsize * sb.free_data_bcnt / 1024. / 1024.;
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
    t = sb.last_wtime;
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
    sprintf(spec_shm + i, "%u\r\n", sb.data_start);
    i += 2 + uint2width(sb.data_start);
    strcpy(spec_shm + i, "上次分配到的 inode 编号：");
    i += 34;
    sprintf(spec_shm + i, "%u\r\n", sb.last_alloc_inode);
    i += 2 + uint2width(sb.last_alloc_inode);
    strcpy(spec_shm + i, "上次分配到的数据块编号：");
    i += 36;
    sprintf(spec_shm + i, "%u", sb.last_alloc_data);
    i += uint2width(sb.last_alloc_data);

    pthread_rwlock_unlock(sb_lock);

    return i;
}

uint16_t cd(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    char cmd_dir[CMD_LEN - 3];
    strcpy(cmd_dir, msg->cmd + 3);
    uint8_t cmd_dir_len = strlen(cmd_dir);
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;
    if (cmd_dir_len == 0)
        return 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        if (cmd_dir_len == 1)
        {
            working_dirs[msg->session_id] = 0;
            return 0;
        }
        working_dir = 0;
        l = r = 1;
    }

    while (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    pthread_rwlock_rdlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "cd: 目录不存在！");
        return 22;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    uint32_t type;
    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, &type, false, false);
    if (ino == -4)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    }
    else if (ino == -1 || type != 1)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "cd: 目录不存在！");
        return 22;
    }
    else
    {
        assert(ino < (uint32_t)-4);

        inode_t inode;
        if (iread(ino, &inode))
        {
            pthread_rwlock_unlock(inode_lock);
            printf("读取 inode 失败！\n");
            strcpy(spec_shm, "ERROR");
            return 5;
        }
        if (!check_privilege(&inode, session_id2uid(msg->session_id), 1))
        {
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cd: 权限不足！");
            return 19;
        }
        pthread_rwlock_unlock(inode_lock);
        working_dirs[msg->session_id] = ino;
    }

    return 0; // 若未出错， cd 不会有字符进入缓冲区。
}

uint16_t ls(const msg_t *const msg)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[msg->session_id];

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
    uint32_t working_dir = working_dirs[msg->session_id];
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    pthread_rwlock_rdlock(inode_lock);
    if (cmd_dir_len == 0)
        goto GetInfo;

    uint8_t l = 0, r = 0;
    if (cmd_dir[0] == '/')
    {
        if (cmd_dir_len == 1)
        {
            working_dirs[msg->session_id] = 0;
            return 0;
        }
        working_dir = 0;
        l = r = 1;
    }

    while (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;

    // 逐一分解 cmd_dir 中出现的目录。
    while (1)
    {
        while (r < cmd_dir_len && cmd_dir[r] != '/')
            r++;

        if (r - l > FILE_NAME_LEN)
        {
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "ls: 目录不存在！");
            return 22;
        }
        char filename[FILE_NAME_LEN];
        memset(filename, 0, FILE_NAME_LEN);
        strncpy(filename, cmd_dir + l, r - l);
        uint32_t type;
        working_dir = search(working_dir, session_id2uid(msg->session_id), filename, &type, false, false);
        if (working_dir == -4)
        {
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "ERROR");
            return 5;
        }
        else if (working_dir == -1 || type != 1)
        {
            pthread_rwlock_unlock(inode_lock);
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
        pthread_rwlock_unlock(inode_lock);
        printf("读取 inode 失败！\n");
        strcpy(spec_shm + i, "ERROR");
        return 5;
    }
    assert(dir_inode.type == 1);
    if (!check_privilege(&dir_inode, session_id2uid(msg->session_id), 4))
    {
        pthread_rwlock_unlock(inode_lock);
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
                    pthread_rwlock_unlock(inode_lock);
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
                        pthread_rwlock_unlock(inode_lock);
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb
                              .blocks[(j - INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK / DIRECT_DIR_ENTRY_CNT],
                          &db))
                {
                    pthread_rwlock_unlock(inode_lock);
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
                        pthread_rwlock_unlock(inode_lock);
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
                        pthread_rwlock_unlock(inode_lock);
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - DOUBLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIRECT_DIR_ENTRY_CNT],
                          &db))
                {
                    pthread_rwlock_unlock(inode_lock);
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
                        pthread_rwlock_unlock(inode_lock);
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
                        pthread_rwlock_unlock(inode_lock);
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
                        pthread_rwlock_unlock(inode_lock);
                        printf("读取目录中间块失败！\n");
                        strcpy(spec_shm, "ERROR");
                        return i > 5 ? i : 5;
                    }
                if (bread(indirectb.blocks[(j - TRIPLE_INDIRECT_DIR_OFFSET) % DIR_ENTRY_PER_INDIRECT_BLOCK /
                                           DIRECT_DIR_ENTRY_CNT],
                          &db))
                {
                    pthread_rwlock_unlock(inode_lock);
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
                pthread_rwlock_unlock(inode_lock);
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
    pthread_rwlock_unlock(inode_lock);
    return i;
}

uint16_t md(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    char cmd_dir[CMD_LEN - 3];
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "md", 2))
        strcpy(cmd_dir, msg->cmd + 3);
    else if (!strncmp(msg->cmd, "mkdir", 5))
        strcpy(cmd_dir, msg->cmd + 6);
    uint8_t cmd_dir_len = strlen(cmd_dir);
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }
    while (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;
    if (cmd_dir_len == 0)
    {
        strcpy(spec_shm, "mkdir: 目录名不能为空！");
        return 31;
    }

    pthread_rwlock_wrlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "mkdir: 目录不存在或目录名过长！");
        return 43;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, NULL, false, false);
    if (ino == -4)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    }
    else if (ino == -1)
    {
        int ret = create_file(working_dir, session_id2uid(msg->session_id), 1, cmd_dir + l);
        pthread_rwlock_unlock(inode_lock);
        switch (ret)
        {
        case -1:
            strcpy(spec_shm, "mkdir: 权限不足！");
            return 22;
        case -2:
            strcpy(spec_shm, "mkdir: 目录容量不足！");
            return 28;
        case -3:
            strcpy(spec_shm, "mkdir: 存储空间不足！");
            return 28;
        case -4:
            strcpy(spec_shm, "mkdir: inode 数量不足！");
            return 28;
        case -5:
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
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "mkdir: 同名目录或文件已存在！");
        return 40;
    }

    return 0; // 若未出错， mkdir 不会有字符进入缓冲区。
}

uint16_t rd(const msg_t *const msg)
{
    bool force = false;
    void *spec_shm = spec_shms[msg->session_id];

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
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }

    while (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;
    if (cmd_dir_len == 0)
    {
        strcpy(spec_shm, "rmdir: 目录名不能为空！");
        return 31;
    }

    pthread_rwlock_wrlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rmdir: 目录不存在！");
        return 25;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, NULL, true, force);
    switch (ino)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rmdir: 目录不存在！");
        return 25;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rmdir: 权限不足！");
        return 22;
    case -3:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rmdir: 目录不为空，可添加 -f 参数强制删除！");
        return 59;
    case -4:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        int ret = remove_dir(ino, session_id2uid(msg->session_id));
        pthread_rwlock_unlock(inode_lock);
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

    return 0; // 若未出错， rmdir 不会有字符进入缓冲区。
}

uint16_t newfile(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    char cmd_dir[CMD_LEN - 8];
    strcpy(cmd_dir, msg->cmd + 8);
    uint8_t cmd_dir_len = strlen(cmd_dir);
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        if (cmd_dir_len == 1)
        {
            working_dirs[msg->session_id] = 0;
            return 0;
        }
        working_dir = 0;
        l = r = 1;
    }

    pthread_rwlock_wrlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "newfile: 目录不存在或文件名过长！");
        return 45;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    if (r == l)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "newfile:文件名不能为空！");
        return 32;
    }
    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, NULL, false, false);
    if (ino == -4)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    }
    else if (ino < (uint32_t)-4)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "newfile: 文件已存在！");
        return 27;
    }
    else
    {
        assert(ino == (uint32_t)-1);
        int ret = create_file(working_dir, session_id2uid(msg->session_id), 0, cmd_dir + l);
        pthread_rwlock_unlock(inode_lock);
        switch (ret)
        {
        case -1:
            strcpy(spec_shm, "newfile: 权限不足！");
            return 24;
        case -2:
            strcpy(spec_shm, "newfile: 目录容量不足！");
            return 30;
        case -3:
            strcpy(spec_shm, "newfile: 存储空间不足！");
            return 30;
        case -4:
            strcpy(spec_shm, "newfile: inode 数量不足！");
            return 30;
        case -5:
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }
    }

    return 0; // 若未出错， rmdir 不会有字符进入缓冲区。
}

uint16_t cat(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    uint32_t page = 0;
    char cmd_dir[CMD_LEN - 3];
    if (!strncmp(msg->cmd + 4, "-p", 2))
    {
        uint8_t i = 7;
        while (true)
        {
            char c = *(msg->cmd + i++);
            if (!('0' <= c && c <= '9'))
                break;
            page = page * 10 + c - '0';
        }
        strcpy(cmd_dir, msg->cmd + i);
    }
    else
        strcpy(cmd_dir, msg->cmd + 4);

    uint8_t cmd_dir_len = strlen(cmd_dir);
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }

    pthread_rwlock_rdlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "cat: 目录或文件不存在！");
        return 32;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    if (r == l)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "cat: 文件名不能为空！");
        return 29;
    }
    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, NULL, false, false);
    switch (ino)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "cat: 文件不存在！");
        return 23;
    case -4:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ino < (uint32_t)-4);
        datablock_t db;
        int ret = read_file(ino, session_id2uid(msg->session_id), page, &db);
        pthread_rwlock_unlock(inode_lock);
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
            memcpy(spec_shm, &db.data, BLOCK_SIZE);
            break;
        }
    }

    return BLOCK_SIZE;
}

uint16_t cp(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    char filename[FILE_NAME_LEN];
    char src[CMD_LEN - 4], dst[CMD_LEN - 4];
    uint32_t ino, dir_ino;
    src[0] = dst[0] = 0;
    uint8_t i = 0;
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "cp", 2))
        i = 3;
    else if (!strncmp(msg->cmd, "copy", 4))
        i = 5;
    uint8_t j = 0;
    while (msg->cmd[i] && msg->cmd[i] != ' ')
        src[j++] = msg->cmd[i++];
    src[j] = 0;
    while (msg->cmd[i] == ' ')
        i++;
    j = 0;
    while (msg->cmd[i] && msg->cmd[i] != ' ')
        dst[j++] = msg->cmd[i++];
    dst[j] = 0;

    uint8_t src_len = strlen(src);
    while (src[src_len - 1] == ' ')
        src[--src_len] = 0;
    if (src_len == 0)
    {
        strcpy(spec_shm, "cp: 源文件不能为空！");
        return 28;
    }
    uint8_t dst_len = strlen(dst);
    while (dst[dst_len - 1] == ' ')
        dst[--dst_len] = 0;
    if (dst_len == 0)
    {
        strcpy(spec_shm, "cp: 目标目录不能为空！");
        return 31;
    }

    bool src_host = false, dst_host = false;
    if (!strncmp(src, "<host>", 6))
        src_host = true;
    if (!strncmp(dst, "<host>", 6))
        dst_host = true;

    pthread_rwlock_wrlock(inode_lock);
    if (!src_host)
    {
        uint8_t l = 0, r = 0;
        uint32_t src_dir = working_dirs[msg->session_id];
        if (src[0] == '/')
        {
            src_dir = 0;
            l = r = 1;
        }

        int ret = locate_last(&l, &r, &src_dir, src_len, src);
        switch (ret)
        {
        case -1:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 源目录或文件不存在！");
            return 34;
            break;
        case -2:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }

        if (r == l)
        {
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 源文件名不能为空！");
            return 31;
        }
        strcpy(filename, src + l);
        uint32_t type;
        ino = search(src_dir, session_id2uid(msg->session_id), src + l, &type, false, false);
        if (type != 0)
        {
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 源文件不存在！");
            return 25;
        }
        switch (ino)
        {
        case -1:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 源文件不存在！");
            return 25;
        case -4:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ino < (uint32_t)-4);
            break;
        }
    }
    if (!dst_host)
    {
        uint8_t l = 0, r = 0;
        uint32_t dst_dir = working_dirs[session_id2uid(msg->session_id)];
        if (dst[0] == '/')
        {
            dst_dir = 0;
            l = r = 1;
        }
        while (dst[dst_len - 1] == '/')
            dst[--dst_len] = 0;
        if (dst_len == 0)
        {
            dir_ino = 0;
            goto Copy;
        }

        int ret = locate_last(&l, &r, &dst_dir, dst_len, dst);
        switch (ret)
        {
        case -1:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 源目录或文件不存在！");
            return 34;
            break;
        case -2:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }

        uint32_t type;
        dir_ino = search(dst_dir, session_id2uid(msg->session_id), dst + l, &type, false, false);
        if (type != 1)
        {
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 目标目录不存在！");
            return 28;
        }
        switch (dir_ino)
        {
        case -1:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "cp: 目标目录不存在！");
            return 28;
        case -4:
            pthread_rwlock_unlock(inode_lock);
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(dir_ino < (uint32_t)-4);
        }
    }
    else if (dst[dst_len - 1] != '/')
    {
        dst[dst_len] = '/';
        dst[++dst_len] = 0;
    }

Copy:
    if (src_host && dst_host)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "cp: 不支持 host 到 host 的复制！");
        return 40;
    }
    else if (!src_host && !dst_host)
    {
        int ret = copy_file(dir_ino, ino, session_id2uid(msg->session_id), filename);
        pthread_rwlock_unlock(inode_lock);
        switch (ret)
        {
        case -1:
            strcpy(spec_shm, "cp: 权限不足！");
            return 19;
        case -2:
            strcpy(spec_shm, "cp: 目录容量不足！");
            return 25;
        case -3:
            strcpy(spec_shm, "cp: 存储空间不足！");
            return 25;
        case -4:
            strcpy(spec_shm, "cp: inode 数量不足！");
            return 25;
        case -5:
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }
    }
    else if (src_host)
    {
        uint8_t l = 7, r = 7;
        // 逐一分解 src 中出现的目录。
        while (true)
        {
            while (r < src_len && src[r] != '/')
                r++;
            if (r == src_len) // 说明已经是文件名。
            {
                if (r == l)
                {
                    pthread_rwlock_unlock(inode_lock);
                    strcpy(spec_shm, "cp: 源文件名不能为空！");
                    return 31;
                }
                if (r - l > FILE_NAME_LEN)
                {
                    pthread_rwlock_unlock(inode_lock);
                    strcpy(spec_shm, "cp: 源文件名过长！");
                    return 25;
                }
                strcpy(filename, src + l);
                break;
            }
            r++;
            l = r;
        }
        int ret = copy_from_host(dir_ino, session_id2uid(msg->session_id), src + 6, filename);
        pthread_rwlock_unlock(inode_lock);
        switch (ret)
        {
        case -1:
            strcpy(spec_shm, "cp: 权限不足！");
            return 19;
        case -2:
            strcpy(spec_shm, "cp: 目录容量不足！");
            return 25;
        case -3:
            strcpy(spec_shm, "cp: 存储空间不足！");
            return 25;
        case -4:
            strcpy(spec_shm, "cp: inode 数量不足！");
            return 25;
        case -5:
            strcpy(spec_shm, "cp: 无法打开宿主文件！");
            return 31;
        case -6:
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }
    }
    else if (dst_host)
    {
        int ret = copy_to_host(ino, session_id2uid(msg->session_id), dst + 6, filename);
        pthread_rwlock_unlock(inode_lock);
        switch (ret)
        {
        case -1:
            strcpy(spec_shm, "cp: 权限不足！");
            return 19;
        case -2:
            strcpy(spec_shm, "cp: 无法创建宿主文件！");
            return 31;
        case -3:
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }
    }
    return 0; // 若未出错， cp 不会有字符进入缓冲区。
}

uint16_t rm(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    char cmd_dir[CMD_LEN - 3];
    // 不会出现 else 场景，以下写法为了代码可读性。
    if (!strncmp(msg->cmd, "rm", 2))
        strcpy(cmd_dir, msg->cmd + 3);
    else if (!strncmp(msg->cmd, "del", 3))
        strcpy(cmd_dir, msg->cmd + 4);

    uint8_t cmd_dir_len = strlen(cmd_dir);
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }

    pthread_rwlock_wrlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rm: 目录或文件不存在！");
        return 31;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    if (r == l)
    {
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rm: 文件名不能为空！");
        return 28;
    }
    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, NULL, true, false);
    switch (ino)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "rm: 文件不存在！");
        return 22;
    case -2:
        strcpy(spec_shm, "rm: 权限不足！");
        return 19;
    case -4:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ino < (uint32_t)-4);
        int ret = remove_file(ino, session_id2uid(msg->session_id));
        pthread_rwlock_unlock(inode_lock);
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

    return 0; // 若未出错， rm 不会有字符进入缓冲区。
}

uint16_t chm(const msg_t *const msg)
{
    void *spec_shm = spec_shms[msg->session_id];

    if (strlen(msg->cmd) < 10)
    {
        strcpy(spec_shm, "chmod: 请正确输入参数和路径！");
        return 40;
    }
    uint32_t privilege = (msg->cmd[6] - '0') * 8 + (msg->cmd[7] - '0');
    if (privilege > 077)
    {
        strcpy(spec_shm, "chmod: 请正确输入参数和路径！");
        return 40;
    }
    char cmd_dir[CMD_LEN - 9];
    strcpy(cmd_dir, msg->cmd + 9);

    uint8_t cmd_dir_len = strlen(cmd_dir);
    while (cmd_dir[cmd_dir_len - 1] == ' ')
        cmd_dir[--cmd_dir_len] = 0;

    uint8_t l = 0, r = 0;
    uint32_t working_dir = working_dirs[msg->session_id];
    if (cmd_dir[0] == '/')
    {
        working_dir = 0;
        l = r = 1;
    }

    while (cmd_dir[cmd_dir_len - 1] == '/')
        cmd_dir[--cmd_dir_len] = 0;
    if (cmd_dir_len == 0)
    {
        strcpy(spec_shm, "chmod: 文件或目录名不能为空！");
        return 40;
    }

    pthread_rwlock_wrlock(inode_lock);

    int ret = locate_last(&l, &r, &working_dir, cmd_dir_len, cmd_dir);
    switch (ret)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "chmod: 目录或文件不存在！");
        return 34;
        break;
    case -2:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ret == 0);
        break;
    }

    uint32_t ino = search(working_dir, session_id2uid(msg->session_id), cmd_dir + l, NULL, false, false);
    switch (ino)
    {
    case -1:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "chmod: 文件或目录不存在！");
        return 34;
    case -4:
        pthread_rwlock_unlock(inode_lock);
        strcpy(spec_shm, "ERROR");
        return 5;
    default:
        assert(ino < (uint32_t)-4);
        int ret = change_privilege(ino, session_id2uid(msg->session_id), privilege);
        pthread_rwlock_unlock(inode_lock);
        switch (ret)
        {
        case -1:
            strcpy(spec_shm, "chmod: 权限不足！");
            return 22;
        case -2:
            strcpy(spec_shm, "ERROR");
            return 5;
        default:
            assert(ret == 0);
            break;
        }
    }

    return 0; // 若未出错， chmod 不会有字符进入缓冲区。
}

uint16_t unknown(uint32_t uid)
{
    uint32_t i = 0;
    void *spec_shm = spec_shms[uid];
    strcpy(spec_shm + i, "无法识别该命令！");
    i += 24;
    return i;
}