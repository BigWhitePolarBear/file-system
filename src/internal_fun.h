#pragma once

#include "common.h"
#include "stdbool.h"

// 返回值为 ino ，若为 0xffffffff 即为异常。
uint32_t get_free_inode();
// 返回值为数据块对应的块号，返回时会加上数据块起始偏移量，若为 0xffffffff 即为异常。
uint32_t get_free_data();

int iread(uint32_t ino, inode_t *const inode);

// 会更新超级块中的修改时间。
int iwrite(uint32_t ino, const inode_t *const inode);

// 超级块的持久化失败时直接退出系统，因此没有返回值。
// 该函数也会更新超级块的最近修改时间。
void sbwrite();
void sbinit();

// 在指定目录中搜寻文件或目录，当找到时，若 delete == true 将其从目录中删除，
// 找到则返回对应的 ino ，否则则返回 0xffffffff ，删除非空目录且 force == false ，
// 返回 0xfffffffe，内部出错时返回 0xfffffffd 。
// 当 type 不为 NULL 时，其将用来存储目标类型（文件或目录）。
uint32_t search(uint32_t dir_ino, const char filename[], uint32_t *type, bool delete, bool force);

// 在指定目录中创建文件或目录，成功返回 0 ，目录容量不足返回 -1 ，
// 内部出错返回 -2 。
int create_file(uint32_t dir_ino, uint32_t uid, uint32_t type, const char filename[]);

// 删除指定文件，成功返回 0 ，若权限不符合返回 -1 ，
// 为目录返回 -2 ，内部出错返回 -3 。
int remove_file(uint32_t ino, uint32_t uid);

// 删除指定目录，成功返回 0 ，若权限不符合返回 -1 ，
// 为文件返回 -2 ，内部出错返回 -3 。
int remove_dir(uint32_t ino, uint32_t uid);

direntry_t get_last_direntry(const inode_t *const dir_inode);

int free_last_directb(inode_t *const dir_inode);
int free_last_indirectb(inode_t *const dir_inode);
int free_last_double_indirectb(inode_t *const dir_inode);
int free_last_triple_indirectb(inode_t *const dir_inode);

uint16_t uint2width(uint32_t num);

// 只计算整数部分。
uint16_t float2width(float num);