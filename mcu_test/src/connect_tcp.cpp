#include "connect_tcp.hpp"
#include <stdio.h>
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

CTcp::CTcp():
	CConnect(DEFAULT_TYPE)
{
	m_fd = -1;
	Connect();
	m_OK = true;
	m_thread = std::thread(&CTcp::Recv,this);
}

CTcp::CTcp(std::string ip, int port, ConnType_e type):
	CConnect(type),m_ip(ip),m_port(port)
{
	m_fd = -1;
	Connect();
	m_OK = true;
	m_thread = std::thread(&CTcp::Recv,this);
}

CTcp::~CTcp()
{
	m_OK = false;
	m_thread.join();
	DisConnect();
}

int CTcp::Connect()
{
	DisConnect();

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < -1)
	{
		perror("socket err");
		return -1;
	}

	printf("socket sucess\n");
#if 0
	int flag = 1;
	int ret =setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
	if (ret == -1) {
		printf("Couldn't setsockopt(TCP_NODELAY)\n");
		return -1;
	}
#endif
	//填充结构体
	struct sockaddr_in clientaddr;
	clientaddr.sin_family = AF_INET;				 //使用ipv4地址
	clientaddr.sin_port = htons(m_port); 	 //主机字节序到网络字节序
	clientaddr.sin_addr.s_addr = inet_addr(m_ip.c_str()); //32位的IP地址

	//3.连接服务器
	int con = connect(sockfd, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
	if (con < 0)
	{
		perror("connect err");
		return -1;
	}
	printf("connect sucess\n");

	m_fd = sockfd;
	
	
	return 0;
}

int CTcp::DisConnect()
{
	
	if(m_fd > 0)
	{
		printf("close fd:%d\n",m_fd);
		close(m_fd);
		m_fd = -1;
	}
}

int CTcp::Send(const uint8_t* data, int len)
{
	if(m_fd < 0)
	{
		return -1;
	}
	
	int ret = send(m_fd, data, len, 0);
	if(ret != len)
	{
		printf("send error2\n");
		return -1;
	}
	
	return 0;
}

bool CTcp::IsConnect()
{
	return (m_fd > 0);
}


void CTcp::Recv()
{
	#define RECV_BUFF_SIZE 1024*10
	uint8_t buff[RECV_BUFF_SIZE] = {0};

	while(m_OK)
	{
		if(m_fd < 0)
		{
			usleep(10*1000);
			Connect();
			continue ;
		}
		
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100*1000;
		setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
		
		int n = recv(m_fd, buff, RECV_BUFF_SIZE, 0);
		if(n > 0)
		{
			if(m_recvCb)
			{
				/*for(int i =0 ;i < n; i++)
				{
					printf("0x%02x ",buff[i]);
				}
				printf("\n");*/
				m_recvCb(buff,n);
			}
		}
		else
		{
			if(errno != EAGAIN)
			{
				printf("recive n:%d error:%s\n",n,strerror(errno));
				DisConnect();
				continue;
			}
		}

		
		
	}

	
}



