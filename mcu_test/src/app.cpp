#include "app.hpp"


int StartUpgrade(CProtocol* proto,const char* filepath,int32_t offset)
{
	FILE *pFile = NULL;
	uint32_t size = 0;
	
	if (NULL == (pFile = fopen(filepath, "r")))
	{
		printf("Open file failed, path:%s.\n", filepath);
		return -1;
	}
	fseek (pFile, 0, SEEK_END);
	size = ftell(pFile) - offset;
	
	uint8_t fileBuff[size] = {0};
	fseek(pFile, offset, SEEK_SET);
	int length = fread(fileBuff, 1, size, pFile);

	if (size != length)
	{
		printf("read %s fail expect %d get: %d: %s\n", filepath, size, length, strerror(errno));
		fclose(pFile);
		return -1;
	}

	fclose(pFile);
	uint32_t totalSize = (uint32_t)(size/2);
	uint32_t crc = 0;
	
	std::vector<uint8_t> response;
	uint8_t tempBuf[128] = {0};
	strcpy((char*)tempBuf, UPGRADE_START_STRING);
	FromUint(totalSize, &tempBuf[strlen(UPGRADE_START_STRING)]);

	printf("*************************start upgrade\n\n");
	int ret = proto->SendSDK(MCU_CMD_START_UPGRADE, strlen(UPGRADE_START_STRING) + sizeof(totalSize),tempBuf ,response,3000);
	if(ret != 0)
	{
		printf("send start upgrade error2\n");
		return -1;
	}
	if(6 == response.size() && 0 == memcmp(&response[0], UPGRADE_READY_STRING, strlen(UPGRADE_READY_STRING)))
	{
		printf("upgrade  start success to %s\n",response[5] == 1 ? "APP_1" : "APP_2");
	}
	else
	{
		printf("mcu response upgrade start error\n");
		return -1;
	}
	
	uint32_t sendSN = 0;
	uint32_t upgradeSN = 0;
	
	uint32_t sendSize = 0;

	while(isOK())
	{
		printf("upgrade send sn:%4d %3d%%\r",sendSN, sendSize*100/totalSize);
		//fflush(stdout);
		
		uint32_t packLen = 64;

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
			break;
		}

		while(isOK() && sendSN - upgradeSN > 16)
		{
			ret = proto->WaitSDK(MCU_CMD_SEND_UPGRADE_DATA, response, 1000);
			if(0 != ret)
			{
				printf("\nwait upgrade sn error\n");
				continue;
			}
			upgradeSN = ToUInt(&response[0]);
		}
		
		sendSN++;
		FromUint(sendSN, &tempBuf[0]);
		memcpy(&tempBuf[sizeof(sendSN)], fileBuff+sendSize, packLen);
		crc = Crc_32(crc,&tempBuf[4],packLen);
		int ret = proto->SendSDK(MCU_CMD_SEND_UPGRADE_DATA,sizeof(sendSN) + packLen,tempBuf);
		if(ret != 0)
		{
			printf("send upgrade data error2\n");
		}
		else
		{
			//printf("%04d %03d%%\r",sendSN,sendSN, sendSize*100/totalSize);
			
		}
		
		sendSize+=packLen;
	}

	
	printf("\nsend end,crc:0x%x\n",crc);
	strcpy((char*)tempBuf, UPGRADE_END_STRING);
	FromUint(crc, &tempBuf[strlen(UPGRADE_END_STRING)]);

	response.clear();
	ret = proto->SendSDK(MCU_CMD_END_UPGRADE, strlen(UPGRADE_END_STRING) + sizeof(crc),tempBuf, response,5000);
	if(ret != 0)
	{
		printf("send upgrade end error\n");
		return -1;
	}
	
	if(1 == response.size() && response[0] == 1)
	{
		printf("upgrade end success***********************\n");
		return 0;
	}
	else
	{
		printf("upgrade end error***********************\n");
		return -1;
	}

}

void SendRTCTime(CProtocol* proto)
{
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
	std::vector<uint8_t> response;
	int ret = proto->SendSDK(MCU_CMD_RTC, 6, date ,response);
	if(ret != 0)
	{
		printf("SendRTCTime error2\n");
		
	}
	else
	{
		printf("send rtc OK %hhu-%hhu-%hhu %hhu:%hhu:%hhu\n",date[0],date[1],date[2],date[3],date[4],date[5]);
	}
	
}

void SendReboot(CProtocol* proto)
{
	uint8_t data[2] = {0x02,0};
	std::vector<uint8_t> response;
	int ret = proto->SendSDK(MCU_CMD_REBOOT, sizeof(data), data,response);
	if(ret != 0)
	{
		printf("SendReboot error2\n");
	}
}

int SendWatchDog(CProtocol* proto,bool open)
{
	uint8_t date[1] = {0x31};//open
	if(!open)
	{
		date[0] = 0x30;//close
	}
	std::vector<uint8_t> response;
	int ret = proto->SendSDK(MCU_CMD_WATCHDOG, 1, date);
	if(ret != 0)
	{
		printf("SendWatchDog error2 \n");
	}
	return ret;
}

void SendShutDown(CProtocol* proto,uint8_t type)
{
	uint8_t date[1] = {type};//open
	int ret = proto->SendSDK(MCU_CMD_SHUTDOWN, 1, date);
	if(ret != 0)
	{
		printf("SendShutDown error2\n");
	}
}

std::string GetVersion(CProtocol* proto)
{
	uint8_t data[3] = {'M','C','U'};
	std::vector<uint8_t> response;
	int ret = proto->SendSDK(MCU_CMD_VERSION, 3, data,response);
	if(ret != 0)
	{
		printf("send GetVersion error2\n");
		return {};
	}

	char version[128] = {0};
	memcpy(version, (char*)&response[0], response.size());
	printf("get version:%s\n",version);
	return std::string(version);
	
}

