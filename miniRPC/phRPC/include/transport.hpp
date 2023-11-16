#pragma once

#include <string>
#include <functional>

namespace phRPC
{

class TransServer
{
	public:
		using callback = std::function<void(std::string& request, std::string& response)>;
		
	protected:
		callback m_callback;
		
	public:		
		TransServer(const std::string& name){};
		virtual ~TransServer() = default;
		
		void SetCallback(callback cb){m_callback = cb;}
};

class TransClient
{	
	public:
		TransClient(const std::string& name){};
		virtual ~TransClient() = default;
		
		//0:success -1:timeout -2:connect failed
		virtual int Request(const std::string& request, std::string& response, uint32_t timeout) = 0;
};


	

}
