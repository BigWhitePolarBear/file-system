#include "common.h"
#include "sys/time.h"
#include "unistd.h"

uint64_t get_timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

uint16_t uint2width(uint32_t num)
{
    if (num == 0)
        return 1;
    uint16_t width = 0;
    while (num > 0)
    {
        width++;
        num /= 10;
    }
    return width;
}

uint16_t float2width(float num)
{
    uint16_t width = 0;
    while (num > 1)
    {
        width++;
        num /= 10;
    }
    return width;
}