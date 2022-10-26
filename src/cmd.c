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

int open_shm()
{
    int fd = shm_open(IN_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        printf("开启共享内存链接失败！\n");
        return -1;
    }
    ftruncate(fd, IN_MSG_SIZE);
    in_shm = mmap(NULL, IN_MSG_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (*(int *)in_shm == -1)
    {
        printf("映射共享内存失败！\n");
        return -1;
    }

    in_mutex = sem_open(IN_SEM_MUTEX_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 1);
    if (in_mutex == SEM_FAILED)
    {
        printf("开启输入互斥信号量失败！\n");
        return -1;
    }
    in_ready = sem_open(IN_SEM_READY_NAME, O_CREAT | O_EXCL, IN_SEM_PERM, 0);
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

void handle_msg()
{
    inmsg_t inmsg;
    while (1)
    {
        if (sem_wait(in_ready))
        {
            printf("获取共享内存互斥信号量失败！\n");
            return;
        }
        inmsg = *(inmsg_t *)in_shm;
        if (inmsg.uid == 0 && strcmp(inmsg.cmd, "shutdown"))
            return;

        sem_post(in_ready);
        usleep(SYNC_WAIT);
    }
}