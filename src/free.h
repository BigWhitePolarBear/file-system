#pragma once

#include "common.h"

// 这些函数用来释放目录 inode 持有的数据块。

int free_last_directb(inode_t *const dir_inode);
int free_last_indirectb(inode_t *const dir_inode);
int free_last_double_indirectb(inode_t *const dir_inode);
int free_last_triple_indirectb(inode_t *const dir_inode);