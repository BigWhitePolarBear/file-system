#include "fcntl.h"
#include "string.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "unistd.h"

#define SHM_NAME "fs_sm"

int main()
{
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 0x100);
    char *p = mmap(NULL, 0x100, PROT_WRITE, MAP_SHARED, fd, 0);
    memset(p, 'A', 0x100);
    munmap(p, 0x100);
    return 0;
}