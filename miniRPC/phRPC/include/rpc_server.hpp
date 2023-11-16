#pragma once

#include <unordered_map>
#include <functional>
#include "function_traits.hpp"
#include "apply.hpp"
#include "args_pack.hpp"
#include "unix_socket.hpp"
#include "md5.hpp"
#include "rpc_err.hpp"

namespace phRPC
{
	
class RPCServer
{
	public:
		RPCServer(const std::string& name):m_name(name)
		{
			m_transport.reset(new USocketServer(m_name));
			m_transport->SetCallback(std::bind(&RPCServer::CallRecive, this, std::placeholders::_1, std::placeholders::_2));
		}
	
		template<typename Fun>
		void Register(Fun f,const std::string& name)
		{
			std::string allname = function_traits<Fun>::Sign(name);
			uint32_t key = MD5Hash32(allname.data());
			m_map[key] = std::bind(&Apply<Fun>, f, std::placeholders::_1, std::placeholders::_2);
		}
		

	private:
		void CallRecive(std::string& request, std::string& response)
		{
			uint32_t key = 0;
			int ret = 0;
			if(BaseUnPack(request,key))
			{
				ret = Call(key,request,response);
			}
			else
			{
				ret = RPC_INVALID_ARGS;
			}
			response = std::move(BasePack(ret) + response);
		}
		
		int Call(uint32_t key, std::string& args, std::string& result)
		{
			if(m_map.count(key))
			{
				return m_map[key](args, result);
			}
			return RPC_NO_FUNCTION;
		}
		
		template<typename Fun,typename T>
		typename std::enable_if<std::is_void<typename function_traits<Fun>::retT>::value>::type
		static CallHelp(Fun& f,T& t,std::string& result)
		{
			apply(f,t);
			result.clear();
		}

		template<typename Fun,typename T>
		typename std::enable_if<!std::is_void<typename function_traits<Fun>::retT>::value>::type
		static CallHelp(Fun& f,T& t, std::string& result)
		{
			auto ret = apply(f,t);
			result = std::move(BasePack(ret));
		}

		template<typename Fun>
		static int Apply(Fun f, std::string& args, std::string& result)
		{
			using tup = typename function_traits<Fun>::tupleT;
			
			tup t;
			if(UnPackArgs(args,t))
			{
				CallHelp(f,t, result);
				return RPC_SUCCESS;
			}
			
			return RPC_INVALID_ARGS;
		}

	private:
		std::unordered_map<uint32_t, std::function<int(std::string&,std::string&)>> m_map;
		std::unique_ptr<USocketServer> m_transport;
		std::string m_name;
};


#define EXPORT_RPC(server,fun)\
server.Register(fun,#fun);

}