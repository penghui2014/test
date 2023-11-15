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
	
class RPCClient
{
	public:
		RPCClient(const std::string& name):m_name(name)
		{
			m_socket.reset(new USocketClient(m_name));
		}
		
		~RPCClient() = default;
		
	public:
		template <typename Fun, uint32_t TIMEOUT = 40, typename... Args>
		typename std::enable_if<!std::is_void<typename function_traits<Fun>::retT>::value, typename function_traits<Fun>::retT>::type
		Call(const std::string& name, Args&&... args)
		{
			using retT = typename function_traits<Fun>::retT;
			using tup = typename function_traits<Fun>::tupleT;
			tup tt = std::forward_as_tuple(std::forward<Args>(args)...);
			retT t;
			std::string allname = MangleFunction<typename function_traits<Fun>::funT>(name);
			uint32_t key = MD5Hash32(name.data());
			
			std::string strArgs = PackArgs(tt);
			std::string result;
			int err = RPC_SUCCESS;
			
			int status = m_socket->Request(key, strArgs, TIMEOUT , result, err);
			if(0 == status && RPC_SUCCESS == err)
			{
				if(!BaseUnPack(result, t))
				{
					err = RPC_INVALID_RETURN;
				}
			}
			
			ChangeErr(status,err);			
			return t;
				
		}
		
		template <typename Fun, uint32_t TIMEOUT = 40,typename... Args>
		typename std::enable_if<std::is_void<typename function_traits<Fun>::retT>::value>::type
		Call(const std::string& name, Args&&... args)
		{
			using retT = typename function_traits<Fun>::retT;
			using tup = typename function_traits<Fun>::tupleT;
			tup tt = std::forward_as_tuple(std::forward<Args>(args)...);
			
			std::string allname = MangleFunction<typename function_traits<Fun>::funT>(name);
			uint32_t key = MD5Hash32(name.data());
			
			std::string strArgs = PackArgs(tt);
			std::string result;
			int err = RPC_SUCCESS;
			
			int status = m_socket->Request(key, strArgs, TIMEOUT , result, err);
			if(0 == status && RPC_SUCCESS == err)
			{
				if(0 != result.length())
				{
					err = RPC_INVALID_RETURN;
				}
			}
			ChangeErr(status,err);
			return ;
		}
		
	private:
		static inline void ChangeErr(int status,int err)
		{
			if(0 == status)
			{
				rpc_err = RPC_SUCCESS;
				if(RPC_SUCCESS != err)
				{
					rpc_err = err;
				}
			}
			else if(-1 == status)
			{
				rpc_err = RPC_TIMEOUT;
			}
			else if(-2 == status)
			{
				rpc_err = RPC_CONNECT_ERR;
			}
		}
		
	private:
		std::string m_name;
		std::unique_ptr<USocketClient> m_socket;

};

}