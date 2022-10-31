#pragma once

#include "cmd.h"
#include "common.h"
#include "stdbool.h"

int login(uint32_t uid, const char pwd[]);

uint16_t info(uint32_t uid);

uint16_t cd(msg_t *msg);

uint16_t ls(uint32_t uid, bool detail);

uint16_t md(msg_t *msg);

uint16_t unknown(uint32_t uid);