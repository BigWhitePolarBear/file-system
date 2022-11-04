#pragma once

#include "common.h"
#include "pthread.h"
#include "stdbool.h"

// 这个头文件和源文件主要包含一些供 internal 等调用的函数，特点是
// 若操作文件或目录，不检查权限是否符合，限制更少，且函数名以_开头，使用前要注意。

extern pthread_rwlock_t **inode_bitmap_locks, **data_bitmap_locks;
extern pthread_rwlock_t *inode_lock, *data_lock, *sb_lock;

int iread(uint32_t ino, inode_t *const inode);

// 会更新超级块中的修改时间。
int iwrite(uint32_t ino, const inode_t *const inode);

void lock_init();

// 超级块的持久化失败时直接退出系统，因此没有返回值。
// 该函数也会更新超级块的最近修改时间。
void sbwrite();
void sbinit();

bool check_privilege(const inode_t *const inode, uint32_t uid, uint32_t privilege);

// 成功返回 0 ，目录容量不足返回 -1 ，存储空间不足返回 -2 ，内部出错返回 -3 。
int _push_direntry(inode_t *const dir_inode, const direntry_t *const de);

// 获取指定目录最后一个目录项，成功返回 0 ，内部出错返回 -1 。
int _get_last_direntry(const inode_t *const dir_inode, direntry_t *const de);

// 读取指定文件的第 page 块数据，成功返回 0 ，内部出错返回 -1 。
int _read_file(const inode_t *const inode, uint32_t page, datablock_t *const db);

// 将数据块追加到文件尾部，成功返回 0 ，存储空间不足返回 -1 ，内部出错返回 -2 。
int _append_block(inode_t *const inode, const datablock_t *const db);