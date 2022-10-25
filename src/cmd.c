#include "cmd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "unistd.h"

void create_sm();
void delete_sm();

void handle_msg()
{
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 0x100);
    char *p = mmap(NULL, 0x100, PROT_READ, MAP_SHARED, fd, 0);
    printf("%c %c\n", p[0], p[255]);
    munmap(p, 0x100);
}