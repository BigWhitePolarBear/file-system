#include "device.h"
#include "assert.h"
#include "common.h"

// device 部分的代码负责硬盘这一块设备的 io 接口模拟，其中会维护 disk
// 作为文件流指针， cur_id 作为当前 disk 指向的块编号，在读写前对其进行
// 相应的调整。

FILE *disk;

uint32_t cur_id;
const uint32_t max_id = DISK_SIZE / BLOCK_SIZE;

int devinit()
{
    disk = fopen("/home/lkh/project/file-system/disk.img", "r+");
    if (!disk)
    {
        printf("打开虚拟磁盘失败！\n");
        return -1;
    }
    cur_id = 0;
    return 0;
}

int devclose()
{
    if (fclose(disk))
    {
        printf("关闭虚拟磁盘失败！\n");
        return -1;
    }
    return 0;
}

int bread(uint32_t id, void *const buf)
{
    assert(id < max_id);

    int offset = (id - cur_id) * BLOCK_SIZE;
    if (fseek(disk, offset, SEEK_CUR))
    {
        printf("虚拟磁盘 fseek 出错！\n");
        return -1;
    }
    if (fread(buf, BLOCK_SIZE, 1, disk) != 1)
    {
        printf("虚拟磁盘 fread 出错！\n");
        return -1;
    }
    cur_id = id + 1;
    return 0;
}

int bwrite(uint32_t id, const void *const buf)
{
    assert(id < max_id);

    int offset = (id - cur_id) * BLOCK_SIZE;
    if (fseek(disk, offset, SEEK_CUR))
    {
        printf("虚拟磁盘 fseek 出错！\n");
        return -1;
    }
    if (fwrite(buf, BLOCK_SIZE, 1, disk) != 1)
    {
        printf("虚拟磁盘 fwrite 出错！\n");
        return -1;
    }
    cur_id = id + 1;
    return 0;
}