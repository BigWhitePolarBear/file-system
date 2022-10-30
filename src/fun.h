#pragma once

#include "common.h"
#include "stdbool.h"

// 返回值为 ino ，若为 0xffffffff 即为异常。
uint32_t get_free_inode();
// 返回值为数据块对应的块号，若为 0xffffffff 即为异常。
uint32_t get_free_data();

int iread(uint32_t ino, inode_t *const inode);
int iwrite(uint32_t ino, const inode_t *const inode);

// 超级块的持久化失败时直接退出系统，因此没有返回值。
// 该函数也会更新超级块的最近修改时间。
void sbwrite();
void sbinit();

int login(uint32_t uid, const char pwd[]);

uint16_t info(uint32_t uid);
uint16_t ls(uint32_t uid, bool detail);
uint16_t unknown(uint32_t uid);

uint16_t num2width(uint32_t num);