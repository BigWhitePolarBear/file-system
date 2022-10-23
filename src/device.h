#pragma once

#include "stdint.h"
#include "stdio.h"

#define DISK_SIZE 100 * 1024 * 1024
#define BLOCK_SIZE 1024

// main 函数开始处应调用 device_init() ，结束时调用 device_close() 。
// 以下的函数调用成功返回 0 ，否则返回 -1 。

int devinit();
int devclose();

int bread(uint32_t id, void *const buf);
int bwrite(uint32_t id, const void *const buf);