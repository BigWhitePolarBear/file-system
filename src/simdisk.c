#include "device.h"
#include "fs.h"

int main()
{
    if (devinit())
    {
        printf("设备初始化出错，退出系统！\n");
        return -1;
    }

    printf("%d\n", mkfs());

    if (devclose())
    {
        printf("设备关闭出错！\n");
        return -1;
    }

    return 0;
}