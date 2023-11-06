#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <string>

#include "protocol.hpp"
#include "connect_tcp.hpp"
#include "common.hpp"
#include "app.hpp"

using namespace std;

bool g_OK = true;

static uint64_t getCurTime()
{
	struct timeval tv;

	gettimeofday(&tv, nullptr);

	return (tv.tv_sec*1e9 + tv.tv_usec*1e3);
}

static uint64_t getRealTime()
{
	timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return (time.tv_sec*1000000000 + (time.tv_nsec));
}

class Fps
{
	private:
		uint64_t m_lastTime;
		uint64_t m_fpsTime;
		int m_count;

		const uint32_t m_size;
		const string m_name;
		const int m_inverval;

	public:
		Fps(string name, uint32_t size,int interval)
			:m_name(name),
			m_size(size),
			m_inverval(interval)
		{

		}
		
		void Put(uint32_t len)
		{
			if(m_size != len)
			{
				printf("%s size error %d need:%d\n",m_name.c_str(),len,m_size);
				return;
			}
			
			m_count++;
			uint64_t curTime = GetMilliSecond();
			if(curTime - m_lastTime > m_inverval)
			{
				printf("%s interval error:%lu\n",m_name.c_str(), curTime-m_lastTime);
			}
			m_lastTime = curTime;
			
			if(curTime - m_fpsTime >= 1000)
			{
				printf("%s fps:%d\n",m_name.c_str(), m_count);
				m_count = 0;
				m_fpsTime = curTime;
			}
		}
};

static void ADRecv(uint8_t msg,uint32_t len,uint8_t* data)
{
	//printf("recv ad msg:%02x len:%hhu\n",msg,len);
	static Fps ctlFps("Control_Out", sizeof(MCU_CTL_OUT_INFO_S), 45);
	static Fps vehFps("Veh_Info", sizeof(LSAD_MCU_VEH_INFO_S), 25);
	 
	switch(msg)
	{
		case MSG_MCU_CTL_OUT_INFO:
		{
			ctlFps.Put(len);
			
			MCU_CTL_OUT_INFO_S* info = (MCU_CTL_OUT_INFO_S*)data;
			static uint32_t s_SN = info->SN;
			if(info->SN != s_SN)
			{
				printf("SN ERROR %d->%d\n",s_SN,info->SN);
			}
			s_SN = info->SN + 1;
		}
		break;

		case MSG_MCU_VEH_INFO:
		{
			vehFps.Put(len);
		}
	}
}

static void ADTest(const char* ip,int port,int interval = 0)
{
	uint64_t lastTime = 0;
	LADS_SOC_CTL_MODE_S cmd;
	LSAD_SOC_PP_INFO_S ppInfo;
	LSAD_SOC_LP_INFO_S locInfo;
	memset(&locInfo,0,sizeof(locInfo));
	memset(&cmd,0,sizeof(cmd));
	memset(&ppInfo,0,sizeof(ppInfo));
	int sendSize = 0;
	uint64_t fefe = 0x7F7E7E7F7D7F7E7D;
	#if 0
	for(int i = 0;i < 200; i++)
	{
		memcpy(&ppInfo.PathPoint_cur[i],&fefe,sizeof(double));
		memcpy(&ppInfo.PathPoint_X[i],&fefe,sizeof(double));
		memcpy(&ppInfo.PathPoint_Y[i],&fefe,sizeof(double));
		memcpy(&ppInfo.PathPoint_Yaw[i],&fefe,sizeof(double));
		memcpy(&ppInfo.PathPoint_vel[i],&fefe,sizeof(double));
		memcpy(&ppInfo.PathPoint_Direction[i],&fefe,sizeof(double));
	}
	#endif
	ppInfo.PreView_Num = 4.0;
	ppInfo.Cur_PreView_Num= 4.0;
	ppInfo.NearPointNum= 4.0;
	ppInfo.Trajectory_Size= 4.0;

	CTcp tcp(ip, port);
	CProtocol proto(&tcp);
	proto.SetADRecvCb(std::bind(&ADRecv, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	
	GetVersion(&proto);
	int ret = 0;

	while (isOK())
	{			
		//proto.GetSendSize();
		ppInfo.SN++;
		ret = proto.SendAD(MSG_SOC_PLANNING_INFO,sizeof(ppInfo),(uint8_t*)&ppInfo);
		if(ret != 0)
		{
			printf("send error1\n");
		}
		//printf("send:%d\n",proto.GetSendSize());
		usleep(1*1000);
		

	    locInfo.SN++;
		ret = proto.SendAD(MSG_SOC_LOCATION_INFO,sizeof(locInfo),(uint8_t*)&locInfo);
		if(ret != 0)
		{
			printf("send error2111\n");
		} 
		usleep(5*1000);
		
		uint64_t cur_time = GetMilliSecond();
		if(cur_time - lastTime > 1000)
		{				
			SendWatchDog(&proto);
			
			cmd.VehicleStop = 0.0;
			int64_t datetime = getCurTime();
			int64_t realtime = getRealTime();
			//printf("datetime:%lu realtime:%lu sub:%lu\n",datetime,realtime,datetime-realtime);
			memcpy(&cmd.SweepingBrushSW,&realtime,8);
			ret = proto.SendAD(MSG_SOC_AUTOMODE_INFO,sizeof(cmd),(uint8_t*)&cmd);
			if(ret != 0)
			{
				printf("send error2\n");
			} 
			
			printf("send:%d cnt:%d\n",proto.GetSendSize(), proto.GetSendCnt());

			lastTime = cur_time;
		}
		
		if(interval > 0)
		{
			usleep(interval*1000);			
		}
	}
}

int main(int argc, char const** argv)
{
	//printf("sizeof(LSAD_SOC_PP_INFO_S):%d\n",sizeof(LSAD_SOC_PP_INFO_S));
	
	if(argc > 3)
	{
		ADTest(argv[1],atoi(argv[2]),atoi(argv[3]));
	}
	else
	{
		ADTest(argv[1],atoi(argv[2]));
	}
	
	return 0;
}