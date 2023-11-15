#pragma once
#include <stdio.h>
#include <stdint.h>

#ifndef __FILENAME__
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

static inline uint64_t GetMilliSecond()
{
    timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec*1000 + (time.tv_nsec/(1000*1000))); // �� ����
}

#define DEBUG_LEVEL(level,fmt,...) if(1 >= level){printf("[%lu~%s:%d:E]" fmt "\n",GetMilliSecond(),__FILENAME__,__LINE__,##__VA_ARGS__);}

#define DEBUG_E(fmt,...) DEBUG_LEVEL(1,fmt,##__VA_ARGS__)

#define DEBUG_D

