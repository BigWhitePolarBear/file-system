#pragma once

#include "common.h"

// 返回 0xffffffff 即为异常。
uint32_t get_free_inode();
// 返回 0xffffffff 即为异常。
uint32_t get_free_data();

int iread(uint32_t ino, inode_t *const inode);
int iwrite(uint32_t ino, const inode_t *const inode);

// 超级块的持久化失败时直接退出系统，因此没有返回值。
// 该函数也会更新超级块的最近修改时间。
void sbwrite();

int login(uint32_t uid, const char pwd[]);