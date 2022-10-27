#include "common.h"
#include "sys/time.h"
#include "unistd.h"

uint64_t get_timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}