#pragma once

#include <string>

namespace miniRPC
{

template<typename T>
struct Mangle;

template<typename Ret,typename FirstArg,typename ...Args>
struct Mangle<Ret(FirstArg,Args...)>
{
	using next = Mangle<Ret(Args...)>;
	static std::string name()
	{
		return typeid(FirstArg).name() + next::name();
	}
};

template<typename Ret>
struct Mangle<Ret()>
{
	static std::string name()
	{
		return {};
	}
};

template<typename ...Args>
std::string MangleCpp(const std::string& name)
{
	return (std::string("_Z") + std::to_string(name.length()) + name + Mangle<Args...>::name());
}



#define MANGLETYPE(type, name) \
(std::string("_Z") + std::to_string(std::string(name).length()) + std::string(name) + Mangle<type>::name())

#define MANGLE(fun) MANGLETYPE(decltype(fun), #fun)
 
 
template<typename ...Args>
struct MangleArg;

template<typename FirstArg,typename ...Args>
struct MangleArg<FirstArg,Args...>
{
	using next = MangleArg<Args...>;
	static std::string name()
	{
		return typeid(FirstArg).name() + next::name();
	}
};

template<>
struct MangleArg<>
{
	static std::string name()
	{
		return {};
	}
};

#define MANGLEARG(fun,...) \
(std::string("_Z") + std::to_string(std::string(#fun).length()) + std::string(#fun) + MangleArg<__VA_ARGS__>::name())

}
