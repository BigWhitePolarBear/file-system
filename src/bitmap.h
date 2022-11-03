#pragma once

#include "common.h"
#include "stdbool.h"

// 修改位图的同时也会修改超级块中的对应数据。
int set_inode_bitmap(uint32_t pos);
// 会将 pos 减去数据块起始偏移量，修改位图的同时也会修改超级块中的对应数据。
int set_data_bitmap(uint32_t pos);
// 修改位图的同时也会修改超级块中的对应数据。
int unset_inode_bitmap(uint32_t pos);
// 会将 pos 减去数据块起始偏移量，修改位图的同时也会修改超级块中的对应数据。
int unset_data_bitmap(uint32_t pos);

bool test_bitblock(const bitblock_t *const bb, uint32_t pos);
void set_bitblock(bitblock_t *const bb, uint32_t pos);
void unset_bitblock(bitblock_t *const bb, uint32_t pos);

// 返回值为 ino ，若为 -1 则 inode 数量不足， -2 则内部错误。
uint32_t get_free_inode();
// 返回值为数据块对应的块号，返回时会加上数据块起始偏移量，若为 -1 则数据块数量不足， -2 则内部错误。
uint32_t get_free_data();