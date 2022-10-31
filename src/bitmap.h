#pragma once

#include "common.h"

// 修改位图的同时也会修改超级块中的对应数据。
int set_inode_bitmap(uint32_t pos);
// 会将 pos 减去数据块起始偏移量，修改位图的同时也会修改超级块中的对应数据。
int set_data_bitmap(uint32_t pos);
// 修改位图的同时也会修改超级块中的对应数据。
int unset_inode_bitmap(uint32_t pos);
// 会将 pos 减去数据块起始偏移量，修改位图的同时也会修改超级块中的对应数据。
int unset_data_bitmap(uint32_t pos);

uint8_t test_bitblock(bitblock_t *bb, uint32_t pos);
void set_bitblock(bitblock_t *bb, uint32_t pos);
void unset_bitblock(bitblock_t *bb, uint32_t pos);