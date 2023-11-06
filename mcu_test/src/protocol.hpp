#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_


#include <stdint.h>
#include <vector>
#include "connect.hpp"
#include "async_message.hpp"



#define		FRAME_DATA_MAX_LEN			10240u


using recv_sdk_f = std::function<void(uint8_t, uint8_t, uint8_t*)>;
using recv_ad_f = std::function<void(uint8_t, uint32_t, uint8_t*)>;

class CProtocol
{
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

	private:
		CConnect* 	m_con;
		FRAME_S 	m_frame;
		RECV_S 		m_recive;
		recv_sdk_f  m_sdkCb;
		recv_ad_f   m_adCb;
		CAsyncMessage m_asyncSDK;
		CAsyncMessage m_asyncAD;
		uint32_t 	m_sendCnt{0};
		uint32_t 	m_sendSize{0};
		uint32_t	m_reciveSize{0};
		
	public:
		CProtocol(CConnect* con);
		virtual ~CProtocol();
		void SetSDKRecvCb(recv_sdk_f cb){m_sdkCb = cb;}
		void SetADRecvCb(recv_ad_f cb){m_adCb = cb;}
		int SendSDK(uint8_t cmd,uint8_t len,uint8_t* body);
		int SendSDK(uint8_t cmd,uint8_t len,uint8_t* body, std::vector<uint8_t>& response,uint32_t timeout = 100);
		int SendAD(uint8_t msgId,uint16_t msgLen,uint8_t* msg);
		int WaitAD(uint8_t msgId, std::vector<uint8_t>& response , uint32_t timeout = 0);
		int WaitSDK(uint8_t msgId,   std::vector<uint8_t>& response, uint32_t timeout = 0);
		uint32_t GetSendSize()
		{
			uint32_t ret = m_sendSize;
			m_sendSize = 0;
			return ret;
		}
		
		uint32_t GetSendCnt()
		{
			uint32_t ret = m_sendCnt;
			m_sendCnt = 0;
			return ret;
		}

	private:
		void PackADMsg(uint8_t msgId,uint16_t msgLen,uint8_t* msg,std::vector<uint8_t>& sendD);
		void PackSDKCmd(uint8_t cmd,uint8_t len,uint8_t* body,std::vector<uint8_t>& sendD);
		void ParseRecv(uint8_t* data, int len);
		int GetFrame(RECV_S& recv,FRAME_S& frame);

};

#endif
