/********
*客户端代码
*client.c
*********/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "protocol.hpp"
#include "connect_tcp.hpp"
#include "common.hpp"
#include "app.hpp"

using namespace std;

bool g_OK = true;

static void my_handler (int param)
{
  	g_OK = false;
}

static void SDKRecv(uint8_t cmd,uint8_t len,uint8_t* data)
{
	printf("recv sdk cmd:%02x len:%hhu\n",cmd,len);
}

void TestUpgrade(const char* ip, int port, const char* file,uint32_t loopCount)
{
	uint32_t upgradeCount = 0;
	uint32_t testCount = loopCount;
	int ret = 0;
	while (isOK() && (testCount--) > 0)
	{
		
		CTcp tcp(ip, port);
		CProtocol proto(&tcp);

		while(isOK() && !tcp.IsConnect())
		{
			sleep(1);
		}
		
		GetVersion(&proto);
		
		ret = SendWatchDog(&proto);
		if(0 != ret)
		{
			sleep(1);
			continue;
		}
	
	
		ret= StartUpgrade(&proto, file);
		if(0 == ret)
		{
			upgradeCount++;
			printf("upgrade count:%u / %u\n",upgradeCount,loopCount);
			SendReboot(&proto);
		}
		sleep(5);
		
	}
}

void TestNormal(const char* ip, int port)
{
	CTcp tcp(ip, port);
	CProtocol proto(&tcp);
	proto.SetSDKRecvCb(std::bind(&SDKRecv, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	while(isOK() && !tcp.IsConnect())
	{
		sleep(1);
	}

	GetVersion(&proto);

	while(isOK())
	{
		int ret = SendWatchDog(&proto);
		if(0 != ret)
		{
			printf("send watch dog error\n");
		}
		sleep(1);
	}
	
}

int main(int argc, char const *argv[])
{
	signal(SIGINT, my_handler);
	signal(SIGPIPE,SIG_IGN);
	if (argc > 3)
	{
		TestUpgrade(argv[1],atoi(argv[2]),argv[3], 0xFFFFFFFF);
	}
	else
	{
		TestNormal(argv[1],atoi(argv[2]));
	}

	return 0;
}

