#pragma once

#include <functional>
#include <tuple>
#include <string>
#include <type_traits>
#include <cxxabi.h>
#include "debug.hpp"

namespace phRPC
{


/* 
* the pack format:2bytes data0 2bytes data1 2bytes data2 ....
*  2bytes means length of next data
*/
#define DEMANGLE_NAME(t) abi::__cxa_demangle(typeid(t).name(),NULL,NULL,NULL) 

template<typename T>
std::string PackBase(const T& t)
{
	return (std::string((char*)&t,sizeof(T)));
}

std::string PackBase(const std::string& t)
{
	return (std::string(t));
}

template<typename T>
std::string Pack(const T& t)
{
	std::string data = std::move(PackBase(t));
	uint16_t size = data.length();
	return (PackBase(size) + data);
}

template<typename Tuple,size_t N>
struct PackHelper
{
	using next = PackHelper<Tuple,N-1>;
	static inline std::string pack(const Tuple& t)
	{
		return next::pack(t) + std::move(Pack(std::get<N-1>(t)));
	}
};

template<typename Tuple>
struct PackHelper<Tuple,1>
{
	static inline std::string pack(const Tuple& t)
	{
		return (Pack(std::get<0>(t)));
	}
};

template<typename Tuple>
struct PackHelper<Tuple,0>
{
	static inline std::string pack(const Tuple& t)
	{
		return {};
	}
};

template<typename ...Args>
std::string Pack(const std::tuple<Args...>& tu)
{
	using tupleType = std::tuple<Args...>;
	return PackHelper<tupleType,sizeof...(Args)>::pack(tu);
}

template<typename T>
bool UnPackBase(const std::string& str, T& t)
{
	if(str.length() != sizeof(T))
	{
		DEBUG_E("length error for:%s len:%d\n",DEMANGLE_NAME(t), (int)str.length());
		return false;
	}
	t = *((T*)str.data());
	return true;
}

bool UnPackBase(const std::string& str,std::string& t)
{
	t = str;
	return true;
}

template<typename T>
bool UnPack(std::string& str,T& t)
{
	uint16_t size;
	try
	{
		if(UnPackBase(str.substr(0,2),size) && UnPackBase(str.substr(2,size),t))
		{
			str = str.substr(2+size);
			return true;
		}
	}
	catch (...)
	{
		DEBUG_E("format error for:%s len:%d",DEMANGLE_NAME(t), (int)str.length());
	}
	return false;
	
}

template<typename Tuple,size_t N>
struct UnPackHelper
{
	using next = UnPackHelper<Tuple,N-1>;
	static inline bool unpack(std::string& str,Tuple& t)
	{
		if(next::unpack(str,t))
		{
			return UnPack(str, std::get<N-1>(t));
		}
		else
		{
			return false;
		}
	}
};

template<typename Tuple>
struct UnPackHelper<Tuple,1>
{
	static inline bool unpack(std::string& str,Tuple& t)
	{
		return UnPack(str, std::get<0>(t));
	}
};

template<typename Tuple>
struct UnPackHelper<Tuple,0>
{
	static inline bool unpack(std::string& str,Tuple& t)
	{
		return (str.length() == 0);
	}
};


template<typename ...Args>
bool UnPack(std::string& str,std::tuple<Args...>& tu)
{
	using tupleType = std::tuple<Args...>;
	return UnPackHelper<tupleType,sizeof...(Args)>::unpack(str,tu);
}

}

