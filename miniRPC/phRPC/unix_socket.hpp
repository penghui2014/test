#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <string.h>

namespace phRPC
{
	
extern __thread int rpc_err;

enum RPC_ERR
{
	RPC_SUCCESS = 0,
	RPC_CONNECT_ERR = -2,
	RPC_TIMEOUT = -3,
	RPC_NO_FUNCTION = -4,
	RPC_INVALID_ARGS = -5,
	RPC_INVALID_RETURN = -6,
};

enum MSG_e
{
	MSG_INVALID,
	MSG_TEST,
	MSG_REQUEST,
	MSG_RESPONSE,
};
	
struct Header
{
	uint64_t seq;
	uint32_t msg;
	uint32_t key;
	int		 err;
	
	Header(){}
	
	Header(uint64_t s,uint32_t m,uint32_t k)
	:seq(s),msg(m),key(k),err(0)
	{
	}
};


class USocketServer
{
	public:
		using callback = std::function<int(uint32_t key, std::string& args, std::string& result)>;
		
	private:
		bool m_OK{false};
		int m_sockFd{-1};
		std::string m_path;
		std::thread m_thread;
		std::mutex m_sockMutex;
		callback m_callback;
		
	public:
		USocketServer(const std::string& name);
		~USocketServer();
		void SetCallback(callback cb){m_callback = cb;}
		
	private:
		void ServerThread();
		bool Init();
		void Exit();
		 
		
};

class USocketClient
{
	private:
		int m_sockFd{-1};
		std::string m_path;
		std::string m_serverPath;
		std::mutex m_sockMutex;
		std::atomic<uint64_t> m_sendSeq{0};
		std::unique_ptr<char> m_buffer;
		
	public:
		USocketClient(const std::string& name);
		~USocketClient();
		//0:success -1:timeout -2:connect failed
		int Request(uint32_t key,const std::string& args, uint32_t timeout ,std::string& result,int& err);
		bool Init();
};


	

}
