#ifndef _RM_LOG_H__
#define _RM_LOG_H__
#include <stdio.h>

#if 0
#ifdef __cplusplus
extern "C"
{
#endif

enum
{
	RM_SDK_LOG_ERROR = 0,		///<
	RM_SDK_LOG_CRITICAL = 1,	///<
	RM_SDK_LOG_WARNING = 2,		///<
	RM_SDK_LOG_MESSAGE = 3,		///<
	RM_SDK_LOG_DEBUG = 4,
};
extern void RM_LOG_Output(const char* tag,int level, const char *file, const char *func, int line, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#define LOG_MODULE "AV.DEVICE"

//日志打印接口
#define LOG_ERROR(fmt...) RM_LOG_Output(LOG_MODULE,RM_SDK_LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt)
#define LOG_CRITICAL(fmt...) RM_LOG_Output(LOG_MODULE,RM_SDK_LOG_CRITICAL, __FILE__, __FUNCTION__, __LINE__, fmt)
#define LOG_WARNING(fmt...) RM_LOG_Output(LOG_MODULE,RM_SDK_LOG_WARNING, __FILE__, __FUNCTION__, __LINE__, fmt)
#define LOG_MESSAGE(fmt...) RM_LOG_Output(LOG_MODULE,RM_SDK_LOG_MESSAGE, __FILE__, __FUNCTION__, __LINE__, fmt)
#define LOG_DEBUG(fmt...) RM_LOG_Output(LOG_MODULE,RM_SDK_LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt)
#else

#define LOG_ERROR(fmt,...) printf("[%s:%s:%d]" fmt "\n",__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_CRITICAL(fmt,...) printf("[%s:%s:%d]" fmt "\n",__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_WARNING(fmt,...) printf("[%s:%s:%d]" fmt "\n",__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_MESSAGE(fmt,...) printf("[%s:%s:%d]" fmt "\n",__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_DEBUG(fmt,...) printf("[%s:%s:%d]" fmt "\n",__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#endif

#endif
