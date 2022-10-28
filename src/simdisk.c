#include "cmd.h"
#include "device.h"
#include "fcntl.h"
#include "format.h"
#include "fun.h"
#include "stdio.h"
#include "termios.h"

int main()
{
    if (devinit())
    {
        printf("设备初始化出错，退出系统！\n");
        return -1;
    }

    struct termios tm, tm_old;
    int fd = 0;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    // 保存当前终端设置
    if (tcgetattr(fd, &tm) < 0)
    {
        printf("获取当前终端状态失败，退出系统！\n");
        return -1;
    }
    tm_old = tm;
    cfmakeraw(&tm); // 原始模式
    if (tcsetattr(fd, TCSANOW, &tm) < 0)
    {
        printf("设置终端状态为原始模式失败！\n");
        return -1;
    }
    uint64_t oldtime = get_timestamp();
    printf("是否进行格式化？按 y 确认（3秒后自动跳过，可按其他键提前跳过）。\r\n");
    while (get_timestamp() - oldtime < 3000)
    {
        char c = getchar();
        switch (c)
        {
        case -1: // 没有输入
            continue;
        case 'y':
        case 'Y':
            printf("开始格式化！\r\n");
            if (mkfs())
            {
                printf("格式化文件系统失败，退出系统！\r\n");
                return -1;
            }
        default:
            oldtime = 0; // 退出循环
        }
    }
    printf("启动系统！\r\n");
    // 恢复原始终端状态。
    fcntl(fd, F_SETFD, O_NONBLOCK);
    if (tcsetattr(fd, TCSANOW, &tm_old) < 0)
    {
        printf("恢复原始终端状态失败，退出系统！\r\n");
        return -1;
    }

    sbinit();

    if (open_shm())
    {
        printf("开启共享内存失败，退出系统！\n");
        return -1;
    }

    printf("启动系统完毕！\r\n");
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