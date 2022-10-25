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

    handle_msg();

    if (devclose())
    {
        printf("设备关闭出错！\n");
        return -1;
    }

    return 0;
}