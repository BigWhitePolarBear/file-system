#include "device.h"

int main()
{
    if (devinit())
        printf("设备初始化出错，退出系统！\n");

    if (devclose())
        printf("设备关闭出错！\n");

    return 0;
}