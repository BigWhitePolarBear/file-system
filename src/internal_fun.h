#pragma once

#include "common.h"
#include "stdbool.h"

// 这个头文件和源文件主要包含一些供 interface 调用的函数，特点是
// 若操作文件或目录，将会要求提供 uid 以检查权限是否符合。

// 在指定目录中搜寻文件或目录，当找到时，若 delete == true 将其从目录中删除，
// 找到则返回对应的 ino ，否则则返回 -1 ，需要删除且权限不足时返回 -2 ，
// 删除非空目录且 force == false ， 返回 -3 ，内部出错时返回 -4 。
// 当 type 不为 NULL 时，其将用来存储目标类型（文件或目录）。
uint32_t search(uint32_t dir_ino, uint32_t uid, const char filename[], uint32_t *type, bool delete, bool force);

// 在指定目录中创建文件或目录，成功返回 0 ，权限不足返回 -1 ，目录容量不足返回 -2 ，
// 存储空间不足返回 -3 ， inode 数量不足返回 -4 ，内部出错返回 -5 。
int create_file(uint32_t dir_ino, uint32_t uid, uint32_t type, const char filename[]);

// 删除指定文件，成功返回 0 ，若权限不符合返回 -1 ，
// 为目录返回 -2 ，内部出错返回 -3 。
int remove_file(uint32_t ino, uint32_t uid);

// 删除指定目录，成功返回 0 ，若权限不符合返回 -1 ，
// 为文件返回 -2 ，内部出错返回 -3 。
int remove_dir(uint32_t ino, uint32_t uid);

// 读取指定文件的第 page 块数据，成功返回 0 ，若权限不符合返回 -1 ，
// 为目录返回 -2 ， page 超出文件大小返回 -3 ，内部出错返回 -4 。
int read_file(uint32_t ino, uint32_t uid, uint32_t page, datablock_t *const db);

// 将指定文件复制到指定目录，成功返回 0 ，若权限不符合返回 -1 ，为目录容量不足返回 -2 ，
// 存储空间不足返回 -3 ， inode 数量不足返回 -4 ，内部出错返回 -5 。
int copy_file(uint32_t dir_ino, uint32_t ino, uint32_t uid, const char filename[]);

// 将指定宿主文件复制到指定目录，成功返回 0 ，若权限不符合返回 -1 ，为目录容量不足返回 -2 ，
// 存储空间不足返回 -3 ， inode 数量不足返回 -4 ，无法打开宿主文件返回 -5 ，内部出错返回 -6 。
int copy_from_host(uint32_t dir_ino, uint32_t uid, const char host_filename[], const char filename[]);

// 将指定宿主文件复制到指定目录，成功返回 0 ，若权限不符合返回 -1 ，无法创建宿主文件返回 -2 ，
// 内部出错返回 -3 。
int copy_to_host(uint32_t ino, uint32_t uid, char host_dirname[], const char filename[]);

// 修改指定目录或文件的权限，成功返回 0 ，权限不符合返回 -1 ，内部出错返回 -2 。
int change_privilege(uint32_t ino, uint32_t uid, uint32_t privilege);