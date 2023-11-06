#include "protocol.hpp"
#include <string.h>

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



CProtocol::CProtocol(CConnect* con):
	m_con(con)
{
	memset(&m_recive,0,sizeof(m_recive));
	m_con->SetRecvCb(std::bind(&CProtocol::ParseRecv, this,std::placeholders::_1, std::placeholders::_2));
}

CProtocol::~CProtocol()
{
	m_con->SetRecvCb(nullptr);
}

int CProtocol::SendSDK(uint8_t cmd,uint8_t len,uint8_t* body)
{
	std::vector<uint8_t> data;
	
	PackSDKCmd(cmd,  len,  body, data);
	m_asyncSDK.AddMsg(cmd);
	int ret = m_con->Send(data.data(), data.size());
	if(0 == ret)
	{
		m_sendCnt++;
		m_sendSize+=data.size();
	}
	return ret;
}

int CProtocol::SendSDK(uint8_t cmd, uint8_t len, uint8_t* body, std::vector<uint8_t>& response, uint32_t timeout)
{
	std::vector<uint8_t> data;
	
	PackSDKCmd(cmd,  len,  body, data);

	m_asyncSDK.AddMsg(cmd);
	
	if(0 == m_con->Send(data.data(), data.size()))
	{
		m_sendCnt++;
		m_sendSize+=data.size();
		return WaitSDK(cmd,response,timeout);
	}
	else
	{
		return -1;
	}
}

int CProtocol::SendAD(uint8_t msgId,uint16_t msgLen,uint8_t* msg)
{
	std::vector<uint8_t> data;
		
	PackADMsg(msgId, msgLen, msg, data);
	/*for(auto d :data)
	{
		printf("%02x ",d);
		
	}
	printf("\n");*/
	int ret = m_con->Send(data.data(), data.size());
	if(0 == ret)
	{
		m_sendCnt++;
		m_sendSize+=data.size();
	}
	return ret;

}

int CProtocol::WaitSDK(uint8_t cmd,std::vector<uint8_t>& data,uint32_t timeout)
{
	return m_asyncSDK.Wait(cmd, data,timeout);
}

int CProtocol::WaitAD(uint8_t msgId,std::vector<uint8_t>& data,uint32_t timeout)
{
	return m_asyncAD.Wait(msgId, data,timeout);
}

void CProtocol::PackADMsg(uint8_t msgId,uint16_t msgLen,uint8_t* msg,std::vector<uint8_t>& sendD)
{
    
    const uint16_t cmdLen = msgLen + 3;
    const uint16_t dataLen = cmdLen + 3;
    const uint8_t cmd = MCU_SPI_CMD;
    
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
}

void CProtocol::PackSDKCmd(uint8_t cmd,uint8_t len,uint8_t* body,std::vector<uint8_t>& sendD)
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
	for(int i = 0; i <	len+3; i++)
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
}

int CProtocol::GetFrame(RECV_S& recv,FRAME_S& frame)
{
	frame.len = 0;
	
	uint16_t i = recv.readIdx;
	for(;i < recv.len + recv.readIdx; i++)
	{
		//printf("0x%02x\n",recv.data[i]);
		if(frame.index >= FRAME_MAX_LEN)
		{
			printf("limit frame len %d,less than %d\n",FRAME_MAX_LEN,frame.index);
			frame.getStart = false;
			frame.getEnd = false;
			frame.len = 0;
			frame.crc = 0;
			frame.index = 0;
			continue;
		}
		
		if(recv.data[i] == MCU_START_FLAG)
		{
			if(frame.getStart)
			{
				printf("ERROR get start already\n");
			}
			frame.len = 0;
			frame.index = 0;
			frame.crc = 0;
			frame.data[frame.index++] = MCU_START_FLAG;
			frame.getStart = true;
		}
		else if(frame.getStart && recv.data[i] == MCU_END_FLAG)
		{
			frame.data[frame.index++] = MCU_END_FLAG;
			frame.getEnd = true;
			break;
		}
		else if (recv.data[i] == MCU_ESCAPE_CHAR && frame.getStart)
		{
			if(i + 1 < recv.readIdx + recv.len)
			{
				i++;
				if (recv.data[i] == MCU_ESCAPE_EXT_1)
				{
					frame.data[frame.index++] = MCU_FRAME_HEADER;
					if(frame.index > 4)
					{
						frame.crc ^= MCU_FRAME_HEADER;
					}
				}
				else if (recv.data[i] == MCU_ESCAPE_EXT_2)
				{
					frame.data[frame.index++] = MCU_FRAME_END;
					if(frame.index > 4)
					{
						frame.crc ^= MCU_FRAME_END;
					}
				}
				else if (recv.data[i] == MCU_ESCAPE_EXT_3)
				{
					frame.data[frame.index++] = MCU_ESCAPE_CHAR;
					if(frame.index > 4)
					{
						frame.crc ^= MCU_ESCAPE_CHAR;
					}
				}
				else
				{
					printf("error char 0x%1x\n",recv.data[i]);
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
		else if(frame.getStart)
		{
			frame.data[frame.index++] = recv.data[i];
			if(frame.index > 4)
			{
				frame.crc ^= recv.data[i];
			}
		}
		else
		{
			//printf("loss data:0x%02x\n",recv.data[i]);
		}
	}
	
	uint16_t consumeLen = (i - recv.readIdx + 1);
	if(consumeLen < recv.len)
	{
		recv.len -= consumeLen;
		recv.readIdx = i + 1;
	}
	else
	{
		recv.len = 0;
		recv.readIdx = 0;
	}
	
	if(frame.getStart && frame.getEnd)
	{
		frame.getStart = false;
		frame.getEnd = false;
		frame.len = frame.index;
		frame.index = 0;
		if(frame.crc != frame.data[3])
		{
			printf("crc error 0x%1x 0x%1x len:%d cmd:%02x\n",frame.crc,frame.data[3],frame.len,frame.data[4]);
		}
		//printf("get data readIdx:%d recv.len:%d %02X\n",recv.readIdx,recv.len,frame.data[recv.readIdx]);
		return consumeLen;
	}
	else if(frame.getStart && !frame.getEnd)
	{
		memmove(&recv.data[0],&recv.data[recv.readIdx],recv.len);
		//printf("need more data %02X\n",recv.data[0]);
		return 0;
	}
	else 
	{
		printf("do not get start and end\n");
		frame.index = 0;
		return -1;
	}
	
}

void CProtocol::ParseRecv(uint8_t* data, int len)
{
	m_reciveSize+=len;
	int leftLen = RECV_BUF_LEN - m_recive.len;
	if(leftLen > len)
	{
		memcpy(&m_recive.data[m_recive.len], data, len);
		m_recive.len+= len;
		
		while(m_recive.len > 0)
		{
			memset(&m_frame,0,sizeof(m_frame));
			if(GetFrame(m_recive, m_frame) > 0)
			{
				if(m_frame.data[4] == 0xF4)
				{
					uint16_t f4Length = m_frame.data[5]*0x100 + m_frame.data[6];
					
					uint8_t msg = m_frame.data[7];
					uint16_t msgLen = m_frame.data[8] + m_frame.data[9]*0x100;
					uint8_t* data = &m_frame.data[10];
					if(f4Length != msgLen + 3)
					{
						printf("len err %d %d\n",f4Length,msgLen);
						continue;
					}
					
					if(m_adCb)
					{
						m_adCb(msg, msgLen, data);
					}	

					m_asyncAD.WakeUp(msg, data, msgLen);
				}
				else
				{
					uint8_t cmd = m_frame.data[4];
					uint8_t len = m_frame.data[5];
					uint8_t* data = &m_frame.data[6];

					if(m_sdkCb)
					{
						m_sdkCb(cmd,len,data);
					}
					m_asyncSDK.WakeUp(cmd, data, len);
				}
			}
			else
			{
				break;
			}
		}
	}

}

