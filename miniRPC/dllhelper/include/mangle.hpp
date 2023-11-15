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

}
