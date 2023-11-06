/********
*客户端代码
*client.c
*********/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <thread>
#include <vector>
#include <csignal>
#include <list>
#include <memory>
#include <thread>             // std::thread
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status
#include "rm_types.h"

#include <inttypes.h>

unsigned long long GetMilliSecond()
{
    timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec*1000 + (time.tv_nsec/(1000*1000))); // �� ����
}

static bool isOK = true;

void my_handler (int param)
{
  isOK = false;
}


#define MCU_START_FLAG   0x7E
#define MCU_END_FLAG     0x7F
#define MCU_CONVERT_FLAG 0x7D
#define MCU_CONVERT_SFLAG 0x01
#define MCU_CONVERT_EFLAG 0x02
#define MCU_CONVERT_CFLAG 0x03

#define MCU_DEST_PATH 0x80
#define MCU_DEVICE_ID 0x01
#define MCU_SPI_CMD   0xF4
#define MCU_VER_CMD   0x04

#define		MCU_ESCAPE_CHAR			0x7D
#define		MCU_ESCAPE_EXT_1		0x01
#define		MCU_ESCAPE_EXT_2		0x02
#define		MCU_ESCAPE_EXT_3		0x03
#define		MCU_FRAME_HEADER		0x7E
#define		MCU_FRAME_END			0x7F

#define RECV_BUF_LEN 20*1024
typedef struct
{
	uint16_t readIdx;
	uint16_t len;
	uint8_t data[RECV_BUF_LEN];
}RECV_S;


#define FRAME_MAX_LEN 10*1024
typedef struct
{
	uint8_t crc;
	uint16_t len;
	uint16_t index;
	bool getStart;
	bool getEnd ;
	uint8_t data[FRAME_MAX_LEN];
}FRAME_S;

static FRAME_S g_frame;
static RECV_S g_recive;

int PackSPI(uint8_t msgId,uint8_t* msg,uint32_t msgLen,std::vector<uint8_t>& sendD)
{
    
    uint16_t cmdLen = msgLen + 3;
    uint16_t dataLen = cmdLen + 3;
    uint8_t  cmd = MCU_SPI_CMD;
    
    uint8_t* data = new uint8_t[dataLen+1];

    data[1] = cmd;
    data[2] = (uint8_t)(cmdLen>>8);
    data[3] = (uint8_t)(cmdLen&0xFF);
    data[4] = msgId;
    data[5] = (uint8_t)(msgLen&0xFF);
    data[6] = (uint8_t)(msgLen>>8);
    
    memcpy(&data[7],msg,msgLen);
    uint8_t crcValue = 0;

    for(int i = 1; i < dataLen; i++)
    {
        crcValue=crcValue^data[i];
    }
    
    data[0] = crcValue;
    //printf("crc:%2X\n",crcValue);
    sendD.clear();
    
    sendD.push_back(MCU_START_FLAG);
    sendD.push_back(MCU_DEST_PATH);
    sendD.push_back(MCU_DEVICE_ID);
    
    for(int i = 0; i <  dataLen+1; i++)
	{
		if(data[i] == MCU_START_FLAG)
		{
			sendD.push_back(MCU_CONVERT_FLAG);
			sendD.push_back(MCU_CONVERT_SFLAG);
		}
		else if(data[i] == MCU_END_FLAG)
		{
			sendD.push_back(MCU_CONVERT_FLAG);
			sendD.push_back(MCU_CONVERT_EFLAG);
		}
		else if(data[i] == MCU_CONVERT_FLAG)
		{
			sendD.push_back(MCU_CONVERT_FLAG);
			sendD.push_back(MCU_CONVERT_CFLAG);
		}
		else
		{
			sendD.push_back(data[i]);
		}
	}
    sendD.push_back(MCU_END_FLAG);
    
    delete [] data;
    return 0;
}
#include <sys/time.h>
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

int getFrame();

int main(int argc, char const *argv[])
{
	signal(SIGINT,my_handler);
	if (argc != 3)
	{
		printf("input %s <ip> <port>\n", argv[0]);
		return -1;
	}
	char buf[128] = "";
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < -1)
	{
		perror("socket err");
		return -1;
	}

	printf("socket sucess\n");

	//填充结构体
	struct sockaddr_in clientaddr;
	clientaddr.sin_family = AF_INET;				 //使用ipv4地址
	clientaddr.sin_port = htons(atoi(argv[2])); 	 //主机字节序到网络字节序
	clientaddr.sin_addr.s_addr = inet_addr(argv[1]); //32位的IP地址

	//3.连接服务器
	int con = connect(sockfd, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
	if (con < 0)
	{
		perror("connect err");
		return -1;
	}
	printf("connect sucess\n");
	std::thread th = std::thread([sockfd](){
		//4.发送数据
		unsigned char buf[] = {0x7E,0x80,0x01,0x73,0xF4,0x00,0x0F,0x86,0x0C,0x00,0x01,0x02,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F};
		unsigned long long last_time = 0;
		int cnt = 0;
		int size = 0;
		unsigned char buffer[] = {0x7E,0x80,0x01,0x37,0x07,0x01,0x31,0x7F};
		LADS_SOC_CTL_MODE_S cmd;
		LSAD_SOC_PP_INFO_S ppInfo;
		LSAD_SOC_LP_INFO_S locInfo;
		memset(&locInfo,0,sizeof(locInfo));
		memset(&cmd,0,sizeof(cmd));
		memset(&ppInfo,0,sizeof(ppInfo));
		int sendSize = 0;
		uint64_t fefe = 0x7F7E7E7F7E7F;
		for(int i = 0;i < 200; i++)
		{
			memcpy(&ppInfo.PathPoint_cur[i],&fefe,sizeof(double));
			memcpy(&ppInfo.PathPoint_X[i],&fefe,sizeof(double));
			memcpy(&ppInfo.PathPoint_Y[i],&fefe,sizeof(double));
			memcpy(&ppInfo.PathPoint_Yaw[i],&fefe,sizeof(double));
			memcpy(&ppInfo.PathPoint_vel[i],&fefe,sizeof(double));
			memcpy(&ppInfo.PathPoint_Direction[i],&fefe,sizeof(double));
		}
		ppInfo.PreView_Num = 4.0;
		ppInfo.Cur_PreView_Num= 4.0;
		ppInfo.NearPointNum= 4.0;
		ppInfo.Trajectory_Size= 4.0;
		sleep(1);
		int ret = send(sockfd, buffer, sizeof(buffer), 0);
		if(ret != sizeof(buffer))
		{
			printf("send error2\n");
		} 
		cnt++;
		sendSize+=sizeof(buffer);
		while (isOK)
		{	
			std::vector<uint8_t> data;
	#if 1
			ret = send(sockfd, buffer, sizeof(buffer), 0);
			if(ret != sizeof(buffer))
			{
				printf("send error2\n");
			} 
			cnt++;
			sendSize+=sizeof(buffer);
			
			
			
			
			
			sendSize+= data.size();			
			usleep(2*1000);
			
			data.clear();	
			ppInfo.SN++;
			PackSPI(0x84,(uint8_t*)&ppInfo,sizeof(ppInfo),data);
			
			ret = send(sockfd, data.data(), data.size(), 0);
			if(ret != data.size())
			{
				printf("send error2\n");
			}
			cnt++;
			sendSize+= data.size();
			usleep(1*1000);
			
			data.clear();
		    locInfo.SN++;
			PackSPI(0x85,(uint8_t*)&locInfo,sizeof(locInfo),data);
			ret = send(sockfd, data.data(), data.size(), 0);
			if(ret != data.size())
			{
				printf("send error2\n");
			} 
			cnt++;
			sendSize+= data.size();
			usleep(5*1000);
			#endif
			
			unsigned long long cur_time = GetMilliSecond();
			if(cur_time - last_time > 1000)
			{				
				data.clear();
				cmd.VehicleStop = 0.0;
				int64_t datetime = getCurTime();
				int64_t realtime = getRealTime();
				printf("datetime:%lu realtime:%lu sub:%lu\n",datetime,realtime,datetime-realtime);
				memcpy(&cmd.SweepingBrushSW,&realtime,8);
				PackSPI(0x87,(uint8_t*)&cmd,sizeof(cmd),data);
				ret = send(sockfd, data.data(), data.size(), 0);
				if(ret != data.size())
				{
					printf("send error2\n");
				} 
				cnt++;
				printf("send:%d cnt:%d\n",sendSize,cnt);
				sendSize = 0;
				cnt = 0;
				last_time = cur_time;
			}
			usleep(500*1000);			
		}
	});

	
	
	unsigned long long last_time = 0;
	int cnt = 0;
	int size = 0;
	struct timeval timeout = {1, 0};
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	int ctlfps = 0;
	int canfps = 0;
	while(isOK)
	{
		uint8_t buff[1024*10];
		//memset(buff, 0, sizeof(buff));
		//printf("start recv\n");
		
        int n = recv(sockfd, buff, 1024*10, 0);
		if(n < 0)
		{
			printf("recive n:%d error:%s\n",n,strerror(errno));
			//isOK = false;
		}
		
		//printf("recv:%d cnt:%d\n",size,cnt);
		if(n > 0)
		{
			size+=n;
			#if 1
			int leftLen = RECV_BUF_LEN - g_recive.len;
			if(leftLen > n)
			{
				memcpy(&g_recive.data[g_recive.len],buff,n);
				g_recive.len+=n;
				while(g_recive.len > 0)
				{
					if(getFrame() > 0)
					{
						cnt++;
						if(g_frame.data[4] == 0xF4)
						{
							uint16_t f4Length = g_frame.data[5]*0x100 + g_frame.data[6];
							
							uint8_t cmd = g_frame.data[7];
							uint16_t cmdLen = g_frame.data[8] + g_frame.data[9]*0x100;
							if(f4Length != cmdLen + 3)
							{
								printf("len err %d %d\n",f4Length,cmdLen);
								continue;
							}
							if(cmd == 0x01)
							{
								if(sizeof(MCU_CTL_OUT_INFO_S) != cmdLen)
								{
									printf("0x01 len error %d need:%d\n",cmdLen,(int)sizeof(MCU_CTL_OUT_INFO_S));
									continue;
								}
								MCU_CTL_OUT_INFO_S* info = (MCU_CTL_OUT_INFO_S*)&g_frame.data[10];
								static uint16_t s_SN = info->SN;
								//printf("get sn:%d\n",info->SN);
								if(info->SN != s_SN)
								{
									printf("SN ERROR %d->%d\n",s_SN,info->SN);
								}
								ctlfps++;
								s_SN = info->SN + 1;
								static uint64_t s_lastTime = GetMilliSecond();
								uint64_t curTime = GetMilliSecond();
								if(curTime - s_lastTime > 45)
								{
									printf("0x01 control cmd interval error:%lu\n",curTime-s_lastTime);
								}
								s_lastTime = curTime;
							}
							else if(cmd == 0x03)
							{
								if(sizeof(LSAD_MCU_VEH_INFO_S) != cmdLen)
								{
									printf("0x03 len error %d need:%d\n",cmdLen,(int)sizeof(LSAD_MCU_VEH_INFO_S));
									continue;
								}
								LSAD_MCU_VEH_INFO_S* pVeh = (LSAD_MCU_VEH_INFO_S*)&g_frame.data[10];
								//printf("DrivingMode:%d\n",pVeh->DrivingMode);
								canfps++;
								static uint64_t s_lastTime = GetMilliSecond();
								uint64_t curTime = GetMilliSecond();
								if(curTime - s_lastTime > 27)
								{
									printf("0x03 can cmd interval error:%lu\n",curTime-s_lastTime);
								}
								s_lastTime = curTime;
							}
						}
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				printf("leftLen ERROR %d %d\n",leftLen,n);
			}
			#endif
		}	
		unsigned long long cur_time = GetMilliSecond();
		if(cur_time - last_time >= 1000)
		{
			last_time = cur_time;
			printf("recv:%d cnt:%d ctlfps:%d canfps:%d\n",size,cnt,ctlfps,canfps);
			size = 0;
			cnt = 0;
			ctlfps= 0;
			canfps = 0;
		}		
		//sleep(20);
	}
	
	//关闭
	th.join();
	//sendThread.join();
	
	close(sockfd);
	return 0;
}

int getFrame()
{
	g_frame.len = 0;
	
	uint16_t i = g_recive.readIdx;
	for(;i < g_recive.len + g_recive.readIdx; i++)
	{
		//printf("0x%02x\n",g_recive.data[i]);
		if(g_frame.index >= FRAME_MAX_LEN)
		{
			printf("limit frame len %d,less than %d\n",FRAME_MAX_LEN,g_frame.index);
			g_frame.getStart = false;
			g_frame.getEnd = false;
			g_frame.len = 0;
			g_frame.crc = 0;
			g_frame.index = 0;
			continue;
		}
		
		if(g_recive.data[i] == MCU_START_FLAG)
		{
			if(g_frame.getStart)
			{
				printf("ERROR get start already\n");
			}
			g_frame.len = 0;
			g_frame.index = 0;
			g_frame.crc = 0;
			g_frame.data[g_frame.index++] = MCU_START_FLAG;
			g_frame.getStart = true;
		}
		else if(g_frame.getStart && g_recive.data[i] == MCU_END_FLAG)
		{
			g_frame.data[g_frame.index++] = MCU_END_FLAG;
			g_frame.getEnd = true;
			break;
		}
		else if (g_recive.data[i] == MCU_ESCAPE_CHAR && g_frame.getStart)
		{
			if(i + 1 < g_recive.readIdx + g_recive.len)
			{
				i++;
				if (g_recive.data[i] == MCU_ESCAPE_EXT_1)
				{
					g_frame.data[g_frame.index++] = MCU_FRAME_HEADER;
					if(g_frame.index > 4)
					{
						g_frame.crc ^= MCU_FRAME_HEADER;
					}
				}
				else if (g_recive.data[i] == MCU_ESCAPE_EXT_2)
				{
					g_frame.data[g_frame.index++] = MCU_FRAME_END;
					if(g_frame.index > 4)
					{
						g_frame.crc ^= MCU_FRAME_END;
					}
				}
				else if (g_recive.data[i] == MCU_ESCAPE_EXT_3)
				{
					g_frame.data[g_frame.index++] = MCU_ESCAPE_CHAR;
					if(g_frame.index > 4)
					{
						g_frame.crc ^= MCU_ESCAPE_CHAR;
					}
				}
				else
				{
					printf("error char 0x%1x\n",g_recive.data[i]);
					break;
				}
			}
			else
			{
				if(i > 0)
				{
					i--;
				}
				
				break;
			}
		}
		else if(g_frame.getStart)
		{
			g_frame.data[g_frame.index++] = g_recive.data[i];
			if(g_frame.index > 4)
			{
				g_frame.crc ^= g_recive.data[i];
			}
		}
		else
		{
			//printf("loss data:0x%02x\n",g_recive.data[i]);
		}
	}
	
	uint16_t consumeLen = (i - g_recive.readIdx + 1);
	if(consumeLen < g_recive.len)
	{
		g_recive.len -= consumeLen;
		g_recive.readIdx = i + 1;
	}
	else
	{
		g_recive.len = 0;
		g_recive.readIdx = 0;
	}
	
	if(g_frame.getStart && g_frame.getEnd)
	{
		g_frame.getStart = false;
		g_frame.getEnd = false;
		g_frame.len = g_frame.index;
		g_frame.index = 0;
		if(g_frame.crc != g_frame.data[3])
		{
			printf("crc error 0x%1x 0x%1x len:%d cmd:%02x\n",g_frame.crc,g_frame.data[3],g_frame.len,g_frame.data[4]);
		}
		//printf("get data readIdx:%d g_recive.len:%d %02X\n",g_recive.readIdx,g_recive.len,g_frame.data[g_recive.readIdx]);
		return consumeLen;
	}
	else if(g_frame.getStart && !g_frame.getEnd)
	{
		memmove(&g_recive.data[0],&g_recive.data[g_recive.readIdx],g_recive.len);
		//printf("need more data %02X\n",g_recive.data[0]);
		return 0;
	}
	else 
	{
		printf("do not get start and end\n");
		g_frame.index = 0;
		return -1;
	}
	
}
