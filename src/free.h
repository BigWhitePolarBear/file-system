#pragma once

#include "common.h"

int free_last_directb(inode_t *const dir_inode);
int free_last_indirectb(inode_t *const dir_inode);
int free_last_double_indirectb(inode_t *const dir_inode);
int free_last_triple_indirectb(inode_t *const dir_inode);