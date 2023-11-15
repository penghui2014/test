#pragma once

#include <unordered_map>
#include <functional>
#include "function_traits.hpp"
#include "apply.hpp"
#include "args_pack.hpp"
#include "mangle.hpp"
#include "unix_socket.hpp"
#include "md5.hpp"

namespace phRPC
{
	
class RPCServer
{
	public:
		RPCServer(const std::string& name):m_name(name)
		{
			m_transport.reset(new USocketServer(m_name));
			m_transport->SetCallback(std::bind(&RPCServer::Call, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}
	
		template<typename Fun>
		void Register(Fun f,const std::string& name)
		{
			std::string allname = MangleFunction<typename function_traits<Fun>::funT>(name);
			uint32_t key = MD5Hash32(name.data());
			m_map[key] = std::bind(&Apply<Fun>, f, std::placeholders::_1, std::placeholders::_2);
			std::cout<<allname<< " key:"<<key<<std::endl;
		}
		

	private:
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
		static call(Fun& f,T& t,std::string& result)
		{
			apply(f,t);
			result.clear();
		}

		template<typename Fun,typename T>
		typename std::enable_if<!std::is_void<typename function_traits<Fun>::retT>::value>::type
		static call(Fun& f,T& t, std::string& result)
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
				call(f,t, result);
				return RPC_SUCCESS;
			}
			
			return RPC_INVALID_ARGS;
		}

	private:
		std::unordered_map<uint32_t, std::function<int(std::string&,std::string&)>> m_map;
		std::unique_ptr<USocketServer> m_transport;
		std::string m_name;
};

}