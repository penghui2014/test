#ifndef _CONNECT_TCP_H_
#define _CONNECT_TCP_H_

#include "connect.hpp"
#include <string>
#include <thread>

#define DEFAULT_IP "192.168.1.201"
#define DEFAULT_PORT 9099

class CTcp:public CConnect
{
	public:
		CTcp();
		CTcp(std::string ip, int port, ConnType_e type = DEFAULT_TYPE);
		virtual ~CTcp();

		int Send(const uint8_t* data, int len) override;
		bool IsConnect() override;
		
	private:
		void Recv();
		int InitSocket();
		int Connect();
		int DisConnect();

	private:
		int m_fd;
		int m_port;
		std::string m_ip;
		std::thread m_thread;
		bool m_OK;
};

#endif