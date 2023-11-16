#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

#include "transport.hpp"
#include "debug.hpp"

namespace phRPC
{
	


enum MSG_e
{
	MSG_INVALID,
	MSG_REQUEST,
	MSG_RESPONSE,
};
	
struct Header
{
	uint64_t seq{0};
	uint32_t msg{MSG_INVALID};
	Header(){}
	Header(uint64_t s,uint32_t m):seq(s),msg(m){}
};


class USocketServer:public TransServer
{
	private:
		bool m_OK{false};
		int m_sockFd{-1};
		std::string m_path;
		std::thread m_thread;
		std::mutex m_sockMutex;

	public:
		USocketServer(const std::string& name);
		~USocketServer();
		
	private:
		void ServerThread();
		bool Init();
		void Exit();		
};

class USocketClient:public TransClient
{
	private:
		int m_sockFd{-1};
		std::string m_path;
		std::string m_serverPath;
		std::mutex m_sockMutex;
		std::atomic<uint64_t> m_sendSeq{0};
		std::unique_ptr<char> m_buffer;
	
	private:
		bool Init();
		
	public:
		USocketClient(const std::string& name);
		~USocketClient();
		int Request(const std::string& request, std::string& response, uint32_t timeout) override;
		
};


typedef struct sockaddr    TSockAddr;
typedef struct sockaddr_un TSockAddrUn;
typedef struct sockaddr_in TSockAddrIn;

static inline void SafeClose(int* sockFd)
{
	if(sockFd != nullptr && *sockFd >= 0)
	{
		close(*sockFd);
		*sockFd = -1;
	}	
}

static inline int Open(const char* path)
{	
	TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));

	unadr.sun_family = AF_LOCAL;
	strcpy(unadr.sun_path, path);

	/* 创建本地socket */
	int sockFd = socket(AF_LOCAL, SOCK_DGRAM, 0);//数据包方式
	if ( sockFd <= 0)
	{
		DEBUG_E("create unix socket:%s fd:%d failed:%s\n",path,sockFd,strerror(errno));
	    return -1;
	}
	//禁止子进程继承fd
	fcntl(sockFd, F_SETFD, FD_CLOEXEC);
	
	unlink(path);

	int iRet = bind(sockFd,(TSockAddr*)&unadr, sizeof(TSockAddr));
	if (iRet != 0)
	{
		DEBUG_E("bind unix socket:%s fd:%d failed:%s\n",path,sockFd,strerror(errno));
		close(sockFd);
		return -1;
	}
	DEBUG_E("init unix socket:%s fd:%d successed\n",path,sockFd);

	return sockFd;
}

		
static inline int WaitReadTimeout(int fd, int millsec)
{
	fd_set rset;
	struct timeval	tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	
	tv.tv_sec = millsec/1000;
	tv.tv_usec= (millsec - tv.tv_sec*1000)*1000;

	return(select(fd+1, &rset, NULL, NULL, &tv));
	/*> 0 有文件描述符可读， =0 超时 < 0 无描述符可读*/
}

static inline std::string CreateTmpPath()
{
	char tmpPath[] = "/tmp/unix_XXXXXX";
	int fd = mkstemp(tmpPath);
	if(fd < 0)
	{
		DEBUG_E("mkstemp error:%s\n",strerror(errno));
		return {};
	}
	close(fd);
	return std::string(tmpPath);
}

static inline std::string GetPidPath(int pid)
{
	std::string str = std::string("/tmp/u_") + std::to_string(pid);
	return str;
}

static inline int Send(int sockFd, const std::string& dstPath, Header& header, const std::string& data)
{
	if(sockFd < 0)
	{
		return -1;
	}
	
	TSockAddrUn unadr;
	struct msghdr hdr;
	struct iovec iov[2];
	
	bzero(&hdr,sizeof(hdr));
	bzero(&unadr,sizeof(unadr));
	bzero(&iov[0],sizeof(iov));
	
	unadr.sun_family = AF_LOCAL;
	strcpy(unadr.sun_path, dstPath.data());
	
	hdr.msg_name = (TSockAddr *)&unadr;
	hdr.msg_namelen = sizeof(TSockAddrUn);
	
	iov[0].iov_base = &header;
	iov[0].iov_len = sizeof(Header);
	iov[1].iov_base = (void*)data.data();
	iov[1].iov_len = data.length();
	
	hdr.msg_iov = iov;
	hdr.msg_iovlen = 2;
	hdr.msg_control = NULL;
	hdr.msg_controllen = 0;

	int ret = sendmsg(sockFd, &hdr, MSG_NOSIGNAL);
	if(ret <= 0)
	{
		DEBUG_E("sendMsg ERROR:%s ret:%d errno:%d\n",strerror(errno),ret,errno);
	}
	
	return ret;
}

static inline int Recive(int sockFd, Header& header, char* recvBuf, int bufLen,int timeOut, std::string& sender)
{
	if(sockFd < 0)
	{	
		return -1;
	}
	
	int out = WaitReadTimeout(sockFd, timeOut);
	if (out <= 0)
	{
		//DEBUG_E("read time out\n");
		return -1;
	}
	
	socklen_t socklen  = sizeof(TSockAddrUn);

	struct msghdr msg;
	memset(&msg,0,sizeof(struct msghdr));
	struct iovec iov[2];

	char control[CMSG_SPACE(sizeof(int))];
	
    TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));

	msg.msg_name = (TSockAddr *)&unadr;
	msg.msg_namelen = socklen;
	iov[0].iov_base = &header;
	iov[0].iov_len = sizeof(header);
	iov[1].iov_base = recvBuf;
	iov[1].iov_len = bufLen;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	
	int recvLen = recvmsg(sockFd , &msg, 0);
	if(recvLen <= 0 )
	{
		if ( (EAGAIN == errno) || (EINTR == errno))
		{
			return 0;   //接收连接超时
		}
		
		DEBUG_E("read error::%s\n",strerror(errno));
		return -1;
	}
	sender = std::string(unadr.sun_path);
	
	return recvLen;
}

USocketServer::USocketServer(const std::string& name)
	:TransServer(name)
{
	m_path = "/tmp/" + name;
	m_OK = true;
	m_thread = std::thread(&USocketServer::ServerThread, this);
}

USocketServer::~USocketServer()
{
	Exit();
}

bool USocketServer::Init()
{	
	if(m_sockFd < 0)
	{
		m_sockFd = Open(m_path.c_str());
	}
	
	return (m_sockFd >= 0);
}

void USocketServer::Exit()
{	
	if(m_thread.joinable())
	{
		m_OK = false;
		m_thread.join();
	}

	SafeClose(&m_sockFd);
}

void USocketServer::ServerThread()
{
	static const uint32_t BUFFER_SIZE = 1024*30;
	static const int headersize = (int)sizeof(Header);
	int ret = -1;
	Init();
	DEBUG_E("Create serverThread");
	char* buffer = new char[BUFFER_SIZE];
	while(m_OK)
	{
		Header header;
		std::string sender;

		ret = Recive(m_sockFd, header, buffer, BUFFER_SIZE, 20, sender);
		if(ret >= headersize)
		{
			if(header.msg != MSG_REQUEST)
			{
				DEBUG_E("msg type error:%d",header.msg);
				continue;
			}
			
			std::string request(buffer, ret - headersize);
			std::string response;
			if(m_callback)
			{
				m_callback(request, response);
			}
			header.msg = MSG_RESPONSE;
			ret = Send(m_sockFd, sender, header, response);
			
			if(ret < 0)
			{
				DEBUG_E("response to %s error %s \n",sender.c_str(), strerror(errno));
			}
		}
		else
		{
			//DEBUG_E("timeout\n");
		}
	}
	
	delete [] buffer;
	
	DEBUG_E("ServerThread exit");
}





USocketClient::USocketClient(const std::string& name):
	TransClient(name)
{
	m_path = CreateTmpPath();
	m_serverPath = "/tmp/" + name;
	Init();
}

USocketClient::~USocketClient()
{
	std::lock_guard<std::mutex> guard_lock(m_sockMutex);
	SafeClose(&m_sockFd);
}

bool USocketClient::Init()
{	
	if(m_sockFd < 0)
	{
		m_sockFd = Open(m_path.c_str());
	}
	
	return (m_sockFd >= 0);
}

int USocketClient::Request(const std::string& request, std::string& response, uint32_t timeout)
{
	static const uint32_t BUFFER_SIZE = 1024*30;
	static const int headersize = (int)sizeof(Header);
	std::lock_guard<std::mutex> guard_lock(m_sockMutex);
	
	if(!Init())
	{
		return -2;
	}
	
	if(nullptr == m_buffer)
	{
		m_buffer.reset(new char[BUFFER_SIZE]);
	}

	uint64_t seq = m_sendSeq.fetch_add(1);
	
	Header header(seq,MSG_REQUEST);
	int ret = Send(m_sockFd, m_serverPath, header, request);
	if(headersize + request.length() == ret)
	{
		std::string sender;
		ret = Recive(m_sockFd, header, m_buffer.get() , BUFFER_SIZE, timeout, sender);
		if(headersize <= ret && m_serverPath == sender && MSG_RESPONSE == header.msg && seq == header.seq)
		{
			response = std::move(std::string(m_buffer.get(), ret - headersize));
			return 0;
		}
		else if(0 == ret)
		{
			DEBUG_E("ERROR path:%s timeout\n",m_serverPath.c_str());
			return -1;
		}
		else
		{
			DEBUG_E("ERROR ret:%d path:%s sender:%s msg:%d seq:%lu %lu\n",ret,m_serverPath.c_str(),sender.c_str(), header.msg, seq, header.seq);
		}
	}
	else
	{
		DEBUG_E("Send error\n");
	}
	
	return -2;
}
	

}
