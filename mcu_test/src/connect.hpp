#ifndef _CONNECT_H_
#define _CONNECT_H_

#include <functional>
#include <stdint.h>

typedef std::function<void(uint8_t*,int len)> recv_cb_f;

enum ConnType_e
{
	DEFAULT_TYPE,
	SDK_TYPE,
	AD_TYPE,
};


class CConnect
{
	protected:
		ConnType_e m_type;
		recv_cb_f m_recvCb;
		
	public:
		CConnect(ConnType_e type):m_type(type),m_recvCb(nullptr){}
		virtual ~CConnect(){};
		void SetRecvCb(recv_cb_f cb){m_recvCb = cb;}
		virtual int Send(const uint8_t* data, int len) = 0;
		virtual bool IsConnect() = 0;
};


#endif