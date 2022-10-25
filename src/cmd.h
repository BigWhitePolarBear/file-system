#pragma once

#define SHM_NAME "fs_sm"

void create_shm();
void delete_shm();

// 通过共享内存获取命令;
void handle_msg();