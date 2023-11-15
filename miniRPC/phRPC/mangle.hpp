#pragma once

#include <string>
#include <cxxabi.h>

namespace phRPC
{

//#define TYPE_NAME(t) abi::__cxa_demangle(typeid(t).name(),NULL,NULL,NULL)
#define TYPE_NAME(t) typeid(t).name()

template<typename T>
struct Mangle;

template<typename Ret,typename FirstArg,typename ...Args>
struct Mangle<Ret(FirstArg,Args...)>
{
	using next = Mangle<Ret(Args...)>;
	static inline std::string mangleArgs()
	{
		return TYPE_NAME(FirstArg) + next::mangleArgs();
	}
	
	static inline std::string mangleReturn()
	{
		return TYPE_NAME(Ret);
	}
	
};

template<typename Ret>
struct Mangle<Ret()>
{
	static std::string mangleArgs()
	{
		return {};
	}
	
	static inline std::string mangleReturn()
	{
		return TYPE_NAME(Ret);
	}
};

template<typename T>
std::string MangleFunction(const std::string& name)
{
	return (Mangle<T>::mangleReturn() + std::string("_Z") + std::to_string(name.length()) + name + Mangle<T>::mangleArgs());
}

}
