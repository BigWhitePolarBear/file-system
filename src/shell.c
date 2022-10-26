#include "cmd.h"
#include "fcntl.h"
#include "semaphore.h"
#include "stdio.h"
#include "string.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "unistd.h"

void *in_shm;

sem_t *in_mutex;
sem_t *in_ready;

// 开辟输入命令的共享内存。
int open_shm();
// 关闭输入命令的共享内存。
int unlink_shm();

int main()
{
    if (open_shm())
    {
        printf("开启共享内存失败，退出系统！\n");
        return -1;
    }

    char c;
    scanf("%c", &c);
    sem_wait(in_mutex);
    memset(in_shm, c, IN_MSG_SIZE);
    sem_post(in_ready);
    usleep(SYNC_WAIT);
    sem_wait(in_ready);
    sem_post(in_mutex);

    return 0;
}

int open_shm()
{
    int fd = shm_open(IN_SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        printf("开启共享内存链接失败！\n");
        return -1;
    }
    in_shm = mmap(NULL, IN_MSG_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if (*(int *)in_shm == -1)
    {
        printf("映射共享内存失败！\n");
        return -1;
    }

    in_mutex = sem_open(IN_SEM_MUTEX_NAME, O_EXCL);
    if (in_mutex == SEM_FAILED)
    {
        printf("开启输入互斥信号量失败！\n");
        return -1;
    }
    in_ready = sem_open(IN_SEM_READY_NAME, O_EXCL);
    if (in_ready == SEM_FAILED)
    {
        printf("开启输入通知信号量失败！\n");
        return -1;
    }

    return 0;
}

int unlink_shm()
{
    if (shm_unlink(IN_SHM_NAME))
    {
        printf("断开共享内存链接失败！\n");
        return -1;
    }
    if (sem_unlink(IN_SEM_MUTEX_NAME))
    {
        printf("断开输入互斥信号量链接失败！\n");
        return -1;
    }
    if (sem_unlink(IN_SEM_READY_NAME))
    {
        printf("断开输入通知信号量链接失败！\n");
        return -1;
    }
    return 0;
}