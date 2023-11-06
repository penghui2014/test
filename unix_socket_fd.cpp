#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <vector>
#include <arm_neon.h>
#include <thread>
#include <dlfcn.h>
#include<iostream>
using namespace std;
//#include "RM_log.h"


typedef struct tagAI_VIDEO_FRAME{
    char *data;
    int len;
    double timestamp;
}AI_VIDEO_FRAME;


int WriteToFile(const char* path, uint8_t* buffer, uint64_t length)
{
    FILE *fp = NULL;
    int write_length = 0;
    fp = fopen(path, "wb+");
    if(fp == NULL)
    {
        printf("open (wb+)%s failed\n",path);
        return -1;
    }
    write_length = fwrite(buffer, 1, length, fp);
    fclose(fp);
    fp = NULL;
    return write_length;
}

static unsigned long long GetMilliSecond()
{
	timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return (time.tv_sec*1000 + (time.tv_nsec/(1000*1000)));
}

typedef	int (*LINK_RM_SDK_AV_AIInit_Nvidia)();
typedef	int (*LINK_RM_SDK_AV_AIUnInit_Nvidia)();
//typedef	int (*LINK_RM_SDK_AV_AIGetFrame_Nvidia)(uint32_t localChn, void* image);
typedef int (*LINK_RM_SDK_AV_AIGetFrameEx)(uint32_t localChn, void** image);
typedef int (*LINK_RM_SDK_AV_AIReleaseFrameEx)(uint32_t localChn, void* image);

LINK_RM_SDK_AV_AIInit_Nvidia RM_SDK_AV_AIInit_Nvidia{nullptr};
LINK_RM_SDK_AV_AIUnInit_Nvidia RM_SDK_AV_AIUnInit_Nvidia{nullptr};
//LINK_RM_SDK_AV_AIGetFrame_Nvidia RM_SDK_AV_AIGetFrame_Nvidia{nullptr};
LINK_RM_SDK_AV_AIGetFrameEx RM_SDK_AV_AIGetFrameEx{nullptr};
LINK_RM_SDK_AV_AIReleaseFrameEx RM_SDK_AV_AIReleaseFrameEx{nullptr};


void ThreadZeroCopy(int chn)
{
	printf("start chn:%d\n",chn);
	
	uint64_t lastTime = GetMilliSecond();
	int cnt = 0;
	AI_VIDEO_FRAME* frame = nullptr;
	
	while(1)
	{
		int fd = 0;
		if(GetMilliSecond() - lastTime >= 1000)
		{
			printf("chn:%d fps:%d\n",chn,cnt);
			cnt = 0;
			lastTime = GetMilliSecond();
		}
		
		if(0 == RM_SDK_AV_AIGetFrameEx(chn,(void**)&frame))
		{
			cnt++;
			static bool save = false;
			if(save == false && chn == 1)
			{
				WriteToFile("out.rgb",(uint8_t*)frame->data,frame->len);
				save = true;
			}
			//if(chn==0)
			{
				//printf("chn:%d timestamp:%lf\n",chn,frame->timestamp);
				//std::cout<<frame.timestamp<<std::endl;
			}
			memset(frame->data,0, 1920*4*20);
			
			usleep(40*1000);
			//break;
			RM_SDK_AV_AIReleaseFrameEx(chn,frame);
			
		}
		else
		{
			//printf("chn:%d get ERROR\n",chn);
			usleep(10*1000);
		}
		
	}
	
	printf("chn %d exit\n",chn);
	
}

/*
void Thread2(int chn)
{
	printf("start chn:%d\n",chn);
	
	uint64_t lastTime = GetMilliSecond();
	int cnt = 0;
	AI_VIDEO_FRAME frame;
	frame.data = new char[1920*1080*3];
	while(1)
	{
		
		int fd = 0;
		
		if(GetMilliSecond() - lastTime >= 1000)
		{
			printf("chn:%d fps:%d\n",chn,cnt);
			cnt = 0;
			lastTime = GetMilliSecond();
		}
		
		if(0 == RM_SDK_AV_AIGetFrame_Nvidia(chn,&frame))
		{
			cnt++;
			static bool save = false;
			if(save == false && chn == 1)
			{
				WriteToFile("out.rgb",(uint8_t*)frame.data,frame.len);
				save = true;
			}
			if(chn==0)
			{
				printf("chn:%d timestamp:%lf\n",chn,frame.timestamp);
				//std::cout<<frame.timestamp<<std::endl;
			}
		}
		else
		{
			//printf("chn:%d get ERROR\n",chn);
			usleep(10*1000);
		}
		
	}
	delete [] frame.data;
	
}
*/

int main()
{
	const char* path = "/usr/local/lib/librmbasic-streamaxsdk.so";
	static const int THREAD_CNT = 2;
	
	void* handle = dlopen(path, RTLD_LAZY|RTLD_GLOBAL);
	const char* dlsymError = dlerror();
	if(dlsymError)
	{
		printf("[ERROR][ERROR][ERROR]dlopen error %s :%s\n",path,dlsymError);
		return 0;
	}
	RM_SDK_AV_AIInit_Nvidia = (LINK_RM_SDK_AV_AIInit_Nvidia)dlsym(handle, "RM_SDK_AV_AIInit_Nvidia");
	RM_SDK_AV_AIUnInit_Nvidia = (LINK_RM_SDK_AV_AIUnInit_Nvidia)dlsym(handle, "RM_SDK_AV_AIUnInit_Nvidia");
	//RM_SDK_AV_AIGetFrame_Nvidia = (LINK_RM_SDK_AV_AIGetFrame_Nvidia)dlsym(handle, "RM_SDK_AV_AIGetFrame_Nvidia");
	RM_SDK_AV_AIGetFrameEx = (LINK_RM_SDK_AV_AIGetFrameEx)dlsym(handle, "RM_SDK_AV_AIGetFrameEx");
	RM_SDK_AV_AIReleaseFrameEx = (LINK_RM_SDK_AV_AIReleaseFrameEx)dlsym(handle, "RM_SDK_AV_AIReleaseFrameEx");
	
	RM_SDK_AV_AIInit_Nvidia();
	
	vector<unique_ptr<thread>> threads(THREAD_CNT);
	
	for(int i = 0;i < THREAD_CNT; i++)
	{
		threads[i].reset(new thread(ThreadZeroCopy,i)) ;
	}
	
	for(int i = 0;i < THREAD_CNT; i++)
	{
		threads[i]->join();
	}
	
	RM_SDK_AV_AIUnInit_Nvidia();
	return 0;
}
