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
		Call(uint32_t key, Args&&... args)
		{
			using retT = typename function_traits<Fun>::retT;
			retT t;
			std::string result;
			
			int ret = CallHelper<Fun,TIMEOUT,Args...>(key, result, std::forward<Args>(args)...);
			if(ret == 0)
			{
				if(!BaseUnPack(result, t))
				{
					rpc_err = RPC_INVALID_RETURN;
				}
			}			
			return t;	
		}
		
		template <typename Fun, uint32_t TIMEOUT = 40,typename... Args>
		typename std::enable_if<std::is_void<typename function_traits<Fun>::retT>::value>::type
		Call(uint32_t key, Args&&... args)
		{
			std::string result;
			int ret = CallHelper<Fun,TIMEOUT,Args...>(key, result, std::forward<Args>(args)...);
			if(ret == 0)
			{
				if(0 != result.length())
				{
					rpc_err = RPC_INVALID_RETURN;
				}
			}
		}
		
	private:		
		template<typename Fun,uint32_t TIMEOUT = 40,typename ...Args>
		inline int CallHelper(uint32_t key, std::string& result, Args&&... args)
		{
			using tup = typename function_traits<Fun>::tupleT;
			tup tt = std::forward_as_tuple(std::forward<Args>(args)...);

			std::string strArgs = PackArgs(tt);
			std::string request = BasePack(key) + strArgs;

			rpc_err = RPC_CONNECT_ERR;
			int status = m_socket->Request(request, result, TIMEOUT);
			if(status == 0)
			{
				BaseUnPack(result,rpc_err);
				return 0;
			}
			else if(-1 == status)
			{
				rpc_err = RPC_TIMEOUT;
			}
			else
			{
				rpc_err = RPC_CONNECT_ERR;
			}
			
			return -1;
		}
		
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


#define IMPORT_RPC(client,fun) \
using type_##fun = decltype(fun); \
using ret_##fun = function_traits<type_##fun>::retT; \
template<typename ...Args> \
ret_##fun fun(Args&&... args) \
{\
	static const uint32_t s_key = MD5Hash32(function_traits<type_##fun>::Sign(#fun).data());\
	return client.Call<type_##fun>(s_key, std::forward<Args>(args)...);\
}

}