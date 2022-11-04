#pragma once

#include "cmd.h"
#include "common.h"
#include "stdbool.h"

int login(uint32_t uid, const char pwd[]);

uint16_t info(uint32_t uid);

uint16_t cd(const msg_t *const msg);

uint16_t ls(const msg_t *const msg);

uint16_t md(const msg_t *const msg);

uint16_t rd(const msg_t *const msg);

uint16_t newfile(const msg_t *const msg);

uint16_t cat(const msg_t *const msg);

uint16_t cp(const msg_t *const msg);

uint16_t rm(const msg_t *const msg);

uint16_t chm(const msg_t *const msg);

uint16_t unknown(uint32_t uid);