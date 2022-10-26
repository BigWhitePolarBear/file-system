#include "cmd.h"
#include "device.h"
#include "format.h"
#include "stdio.h"

int main()
{
    if (devinit())
    {
        printf("设备初始化出错，退出系统！\n");
        return -1;
    }
    if (open_shm())
    {
        printf("开启共享内存失败，退出系统！\n");
        return -1;
    }
    handle_msg();

    int ret = 0;
    if (unlink_shm())
    {
        printf("解除共享内存链接失败！\n");
        ret = -1;
    }
    if (devclose())
    {
        printf("设备关闭出错！\n");
        ret = -1;
    }
    return ret;
}