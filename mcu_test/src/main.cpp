/********
*客户端代码
*client.c
*********/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
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
using namespace std;

uint32_t crc32utils(uint32_t crc, const uint8_t *buffer, uint32_t size);
unsigned int Crc_32(unsigned int crc, const unsigned char *pBuffer, unsigned int len);
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

enum
{
	MCU_CMD_START_UPGRADE          = 0x01, /* MCU升级开始 */
    MCU_CMD_SEND_UPGRADE_DATA      = 0x02, /* MCU升级数据 */
    MCU_CMD_END_UPGRADE            = 0x03, /* MCU升级结束 */
};

int PackMCU(uint8_t cmd,uint8_t len,uint8_t* body,std::vector<uint8_t>& sendD)
{
	uint8_t* data = new uint8_t[len+3];
	memset(data,0,len+3);
	data[1] = cmd;
	data[2] = len;
	memcpy(&data[3],body,len);
	
	for(int i = 1; i < len+3; i++)
    {
        data[0]^=data[i];
    }
	sendD.clear();
	sendD.push_back(MCU_START_FLAG);
    sendD.push_back(MCU_DEST_PATH);
    sendD.push_back(MCU_DEVICE_ID);
	for(int i = 0; i <  len+3; i++)
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
    /*for(int i = 0;i < sendD.size(); i++)
	{
		printf("%02X ",sendD[i]);
	}
	printf("\n\n");*/
	
    delete [] data;
    return 0;
}

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
    printf("crc:%2X\n",crcValue);
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
static uint32_t g_upgradeSN = 0;
static uint32_t g_upgradeCMD = 0;

void StartUpgrade(int sockfd,const char* filepath,int32_t offset = 180)
{
	//const char *filepath = "./LADS-M01-TC397-MCU-T22122910.CRC";
	FILE *pFile = NULL;
	uint32_t size = 0;
	
	
	if (NULL == (pFile = fopen(filepath, "r")))
	{
		printf("Open file failed, path:%s.\n", filepath);
		return;
	}
	fseek (pFile, 0, SEEK_END);
	size = ftell(pFile) - offset;
	
	uint8_t buffer[size] = {0};
	fseek(pFile, offset, SEEK_SET);
	int length = fread(buffer, 1, size, pFile);

	if (size != length)
	{
		printf("read %s fail expect %d get: %d: %s\n", filepath, size, length, strerror(errno));
		fclose(pFile);
		return ;
	}

	fclose(pFile);
	uint32_t totalSize = (uint32_t)(size/2);
	uint32_t crc = 0;
	
	//printf("crc:0x%x\n",crc);
	uint8_t tempBuf[128] = {0};
	memcpy(tempBuf,"START",5);
	tempBuf[5] = (totalSize >> 24) & 0xFF;
	tempBuf[6] = (totalSize >> 16) & 0xFF;
	tempBuf[7] = (totalSize >> 8) & 0xFF;
	tempBuf[8] = totalSize & 0xFF;
	std::vector<uint8_t> sendD;
	printf("*************************start upgrade\n\n");
	PackMCU(MCU_CMD_START_UPGRADE,9,tempBuf,sendD);
	int ret = send(sockfd, sendD.data(), sendD.size(), 0);
	if(ret != sendD.size())
	{
		printf("send error2\n");
	} 
	while(1)
	{
		if(g_upgradeCMD == MCU_CMD_START_UPGRADE)
		{
			break;
		}
		usleep(50*1000);
	}
	
	uint32_t sendSN = 1;
	uint32_t packLen = 0;
	uint32_t sendSize = 0;
	char body[68] = {0};
	while(1)
	{
		if(totalSize - sendSize > 64)
		{
			packLen = 64;
		}
		else if(totalSize  > sendSize)
		{
			packLen = totalSize - sendSize;
		}
		else
		{
			printf("send end\n");
			memcpy(tempBuf,"END",3);
			printf("crc:0x%x\n",crc);
			tempBuf[3] = (crc >> 24) & 0xFF;;
			tempBuf[4] = (crc >> 16) & 0xFF;
			tempBuf[5] = (crc >> 8) & 0xFF;
			tempBuf[6] = crc & 0xFF;
		
			PackMCU(MCU_CMD_END_UPGRADE,7,tempBuf,sendD);
			int ret = send(sockfd, sendD.data(), sendD.size(), 0);
			if(ret != sendD.size())
			{
				printf("send error2\n");
				
			}
			while(g_upgradeCMD != MCU_CMD_END_UPGRADE)
			{
				usleep(50*1000);
			}
			printf("upgrade end***********************\n");
			break;
		}
		
		while(sendSN - g_upgradeSN > 16)
		{
			usleep(50*1000);
		}
		
		tempBuf[0] = (sendSN >> 24) & 0xFF;
		tempBuf[1] = (sendSN >> 16) & 0xFF;
		tempBuf[2] = (sendSN >> 8) & 0xFF;
		tempBuf[3] = sendSN & 0xFF;
		memcpy(&tempBuf[4],buffer+sendSize,packLen);
		crc = Crc_32(crc,&tempBuf[4],packLen);
		PackMCU(MCU_CMD_SEND_UPGRADE_DATA,4+packLen,tempBuf,sendD);
		int ret = send(sockfd, sendD.data(), sendD.size(), 0);
		if(ret != sendD.size())
		{
			printf("send error2\n");
			
		}
		else
		{
			printf("send sn:%d\n",sendSN);
		}
		/*while(1)
		{
			printf("wait sn:%d\n",sendSN);
			if(g_upgradeSN == sendSN)
			{
				break;
			}
			usleep(50*1000);
		}*/
		sendSN++;
		sendSize+=packLen;
	}
}

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
	bool getStart;
	bool getEnd ;
	uint8_t data[FRAME_MAX_LEN];
}FRAME_S;

void sendDateTime(int sockfd)
{
	vector<uint8_t> sendD;
	tm tm_tmp;
	struct timeval tv;
	gettimeofday(&tv, nullptr);
    gmtime_r(&tv.tv_sec, &tm_tmp);
	uint8_t date[6] = {0};
	
	date[0] = (tm_tmp.tm_year + 1900 - 2000);
	date[1] = (tm_tmp.tm_mon + 1);
	date[2] = (tm_tmp.tm_mday);
	
	date[3] = (tm_tmp.tm_hour);
	date[4] = (tm_tmp.tm_min);
	date[5] = (tm_tmp.tm_sec);
	
	PackMCU(0x08,6,date,sendD);
	int ret = send(sockfd, sendD.data(), sendD.size(), 0);
	if(ret != sendD.size())
	{
		printf("send error2\n");
		
	}
	else
	{
		printf("send rtc OK %hhu-%hhu-%hhu %hhu:%hhu:%hhu\n",date[0],date[1],date[2],date[3],date[4],date[5]);
	}
	
}

static FRAME_S g_frame = {0};
static RECV_S g_recive = {0};
int getFrame();
int main(int argc, char const *argv[])
{
	signal(SIGINT,my_handler);
	signal(SIGPIPE,SIG_IGN);
	if (argc < 3)
	{
		printf("input %s <ip> <port> <updateImg>\n", argv[0]);
		return -1;
	}
	char buf[128] = "";
	//1.创建流式套接字 socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < -1)
	{
		perror("socket err");
		return -1;
	}
	int flag = 1;
	int ret =setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
	if (ret == -1) {
		printf("Couldn't setsockopt(TCP_NODELAY)\n");
		exit(-1);
	}
	
	printf("socket sucess\n");
	memset(&g_frame,0,sizeof(g_frame));
	memset(&g_recive,0,sizeof(g_recive));
	//2
	//填充结构体
	struct sockaddr_in clientaddr;
	clientaddr.sin_family = AF_INET;				 //使用ipv4地址
	clientaddr.sin_port = htons(atoi(argv[2]));		 //主机字节序到网络字节序
	clientaddr.sin_addr.s_addr = inet_addr(argv[1]); //32位的IP地址

	//3.连接服务器
	int connect_t;
	connect_t = connect(sockfd, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
	if (connect_t < 0)
	{
		perror("connect err");
		return -1;
	}
	printf("connect sucess\n");
	std::thread th = std::thread([sockfd,argc,argv](){
		//4.发送数据
		unsigned long long last_time = 0;
		int cnt = 0;
		int size = 0;
		unsigned char buffer[] = {0x7E,0x80,0x01,0x37,0x07,0x01,0x31,0x7F};
		uint8_t reboot_cm[] = {0x7E,0x80,0x01,0x05,0x06,0x01,0x02,0x7F};
		uint8_t shutdowm_cm[] = {0x7E,0x80,0x01,0x37,0x05,0x03,0x31,0x00,0x00,0x7F};
		sleep(1);
		while (isOK)
		{
			int ret = 0;
			ret = send(sockfd, buffer, sizeof(buffer), 0);
			if(ret != sizeof(buffer))
			{
				printf("send error2\n");
			} 
			sendDateTime(sockfd);
			#if 0
			ret = send(sockfd, shutdowm_cm, sizeof(shutdowm_cm), 0);
			if(ret != sizeof(shutdowm_cm))
			{
				printf("send error2\n");
			} 
			#endif
			usleep(200*1000);
			if(argc > 3)
			{
				 StartUpgrade(sockfd,argv[3]);
				 while(isOK)
				 {
					 usleep(10*1000);;
				 }
			}
		}
	});

	//sleep(1);
	//std::thread sendThread = std::thread(sendPPThread);
	
	unsigned long long last_time = 0;
	int cnt = 0;
	int size = 0;
	struct timeval timeout = {1, 0};
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	while(isOK)
	{
		uint8_t buff[1024*3];
        int n = recv(sockfd, buff, 1024*3, 0);
		//printf("recv:%d g_recive.len:%d\n",n,g_recive.len);
		if(n < 0)
		{
			printf("recive n:%d error:%s\n",n,strerror(errno));
			//isOK = false;
		}
	
		//printf("recv:%d cnt:%d\n",size,cnt);
		if(n > 0)
		{
			cnt++;
			size+=n;
			uint16_t recvLen = (uint16_t)n;
			uint16_t leftLen = RECV_BUF_LEN - g_recive.len;
			if(leftLen > recvLen)
			{
				memcpy(&g_recive.data[g_recive.len],buff,recvLen);
				
				g_recive.len+=recvLen;
				
				while(g_recive.len > 0)
				{
					if(getFrame() > 0)
					{
						if(g_frame.data[4] == MCU_CMD_SEND_UPGRADE_DATA)
						{
							if(g_frame.data[5] == 5)
							{
								char* tmp = (char*)&g_upgradeSN;
								tmp[3] = g_frame.data[6];
								tmp[2] = g_frame.data[7];
								tmp[1] = g_frame.data[8];
								tmp[0] = g_frame.data[9];
								//printf("g_upgradeSN:%d\n",g_upgradeSN);
							}
							else
							{
								printf("0x02 len error:%d\n",g_frame.data[5]);
							}
						}
						else if(g_frame.data[4] == MCU_CMD_START_UPGRADE)
						{
							g_upgradeCMD = MCU_CMD_START_UPGRADE;
						}
						else if(MCU_CMD_END_UPGRADE == g_frame.data[4])
						{
							g_upgradeCMD = MCU_CMD_END_UPGRADE;
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
				printf("leftLen ERROR %d %d %d\n",leftLen,n,g_recive.len);
			}
		}	
		unsigned long long cur_time = GetMilliSecond();
		if(cur_time - last_time > 1000)
		{
			last_time = cur_time;
			printf("recv:%d cnt:%d g_recive.len:%d\n",size,cnt,g_recive.len);
			size = 0;
			cnt = 0;
		}		
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
		if(g_frame.len >= FRAME_MAX_LEN)
		{
			printf("limit frame len %d,less than %d\n",FRAME_MAX_LEN,g_frame.len);
			g_frame.getStart = false;
			g_frame.getEnd = false;
			g_frame.len = 0;
			g_frame.crc = 0;
			continue;
		}
		
		if(g_recive.data[i] == MCU_START_FLAG)
		{
			if(g_frame.getStart)
			{
				printf("ERROR get start already\n");
			}
			g_frame.len = 0;
			g_frame.crc = 0;
			g_frame.data[g_frame.len++] = MCU_START_FLAG;
			g_frame.getStart = true;
		}
		else if(g_frame.getStart && g_recive.data[i] == MCU_END_FLAG)
		{
			g_frame.data[g_frame.len++] = MCU_END_FLAG;
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
					g_frame.data[g_frame.len++] = MCU_FRAME_HEADER;
					if(g_frame.len > 4)
					{
						g_frame.crc ^= MCU_FRAME_HEADER;
					}
				}
				else if (g_recive.data[i] == MCU_ESCAPE_EXT_2)
				{
					g_frame.data[g_frame.len++] = MCU_FRAME_END;
					if(g_frame.len > 4)
					{
						g_frame.crc ^= MCU_FRAME_END;
					}
				}
				else if (g_recive.data[i] == MCU_ESCAPE_EXT_3)
				{
					g_frame.data[g_frame.len++] = MCU_ESCAPE_CHAR;
					if(g_frame.len > 4)
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
			g_frame.data[g_frame.len++] = g_recive.data[i];
			if(g_frame.len > 4)
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
		if(g_frame.crc != g_frame.data[3])
		{
			printf("crc error 0x%1x 0x%1x\n",g_frame.crc,g_frame.data[3]);
		}
		//printf("get data readIdx:%d g_recive.len:%d %02X\n",g_recive.readIdx,g_recive.len,g_frame.data[g_recive.readIdx]);
		return consumeLen;
	}
	else if(g_frame.getStart && !g_frame.getEnd)
	{
		memmove(&g_recive.data[0],&g_recive.data[g_recive.readIdx],g_recive.len);
		printf("need more data\n");
		return 0;
	}
	else 
	{
		//printf("do not get start and end\n");
		return -1;
	}
	
}


const static uint32_t crc32_table[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t crc32utils(uint32_t crc, const uint8_t *buffer, uint32_t size)
{
    for (int32_t i = 0; i < size; i++)
    {
        crc = crc32_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

unsigned int Crc_32(unsigned int crc, const unsigned char *pBuffer, unsigned int len)
{
    const unsigned char *s = pBuffer;

    static const unsigned long crc32_table[256] = {
        0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
        0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
        0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
        0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
        0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
        0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
        0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
        0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
        0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
        0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
        0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
        0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
        0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
        0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
        0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
        0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
        0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
        0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
        0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
        0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
        0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
        0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
        0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
        0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
        0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
        0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
        0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
        0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
        0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
        0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
        0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
        0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
        0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
        0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
        0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
        0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
        0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
        0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
        0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
        0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
        0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
        0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
        0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
        0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
        0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
        0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
        0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
        0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
        0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
        0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
        0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
        0x2d02ef8dL
    };


    while(len)
    {
        crc = crc32_table[(crc ^ *s++) & 0xff] ^ (crc >> 8);
        len --;
    }

    return crc;
}