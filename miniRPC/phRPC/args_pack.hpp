#pragma once

#include <functional>
#include <tuple>
#include <string>
#include <type_traits>
#include <cxxabi.h>

namespace phRPC
{
	
template<typename T>
std::string Pack(const T& t)
{
	return std::move(std::string((char*)&t,sizeof(T)));
}

std::string Pack(const std::string& t)
{
	return std::move(std::string(t));
}

template<typename T>
bool UnPack(std::string& str, T& t)
{
	if(str.length() != sizeof(T))
	{
		std::cout<<"length error 1 for:" << 
			abi::__cxa_demangle(typeid(t).name(),NULL,NULL,NULL) 
			<<" len:"<<str.length()<< std::endl;
		return false;
	}
	t = *((T*)str.c_str());
	return true;
}

bool UnPack(std::string& str,std::string& t)
{
	t = str;
	return true;
}

template<typename T>
std::string BasePack(T& t)
{
	std::string data = std::move(Pack(t));
	uint16_t size = data.length();
	return std::move(Pack(size) + data);
}

template<typename Tuple,size_t N>
struct PackHelper
{
	using next = PackHelper<Tuple,N-1>;
	static inline std::string pack(Tuple& t)
	{
		return next::pack(t) + std::move(BasePack(std::get<N-1>(t)));
	}
};

template<typename Tuple>
struct PackHelper<Tuple,1>
{
	static inline std::string pack(Tuple& t)
	{
		return std::move(BasePack(std::get<0>(t)));
	}
};

template<typename Tuple>
struct PackHelper<Tuple,0>
{
	static inline std::string pack(Tuple& t)
	{
		return {};
	}
};

template<typename ...Args>
std::string PackArgs(std::tuple<Args...> tu)
{
	using tupleType = std::tuple<Args...>;
	return PackHelper<tupleType,sizeof...(Args)>::pack(tu);
}

template<typename T>
bool BaseUnPack(std::string& str,T& t)
{
	if(str.length() < 2)
	{
		std::cout<<"format error\n";
		return false;
	}
	uint16_t size = *((uint16_t*)str.substr(0,2).c_str());
	if(str.length() < (2 + size))
	{
		std::cout<<"length error1\n";
		return false;
	}
	std::string data = str.substr(2,size);
	bool ret = UnPack(data,t);
	str = str.substr(2+size);
	return ret;
}

template<typename Tuple,size_t N>
struct UnPackHelper
{
	using next = UnPackHelper<Tuple,N-1>;
	static inline bool unpack(std::string& str,Tuple& t)
	{
		if(next::unpack(str,t))
		{
			return BaseUnPack(str, std::get<N-1>(t));
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
		return BaseUnPack(str, std::get<0>(t));
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
bool UnPackArgs(std::string& str,std::tuple<Args...>& tu)
{
	using tupleType = std::tuple<Args...>;
	return UnPackHelper<tupleType,sizeof...(Args)>::unpack(str,tu);
}

}

