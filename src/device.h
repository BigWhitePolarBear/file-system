#pragma once

#include "stdint.h"

// main 函数开始处应调用 devinit() ，结束时调用 devclose() ，这构成了最基本的设备层抽象。
// 以下的函数调用成功返回 0 ，否则返回 -1 。

int devinit();
int devclose();

int bread(uint32_t id, void *const buf);
int bwrite(uint32_t id, const void *const buf);