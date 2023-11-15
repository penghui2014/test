#pragma once

#include <string>
#include <functional>
#include "dllHelp.hpp"

namespace miniRPC
{

template <typename T> 
struct Wrapper;

template <typename Ret, typename... Args>
struct Wrapper<Ret(Args...)> 
{
	using type = Ret(Args...);
	using result = Ret;

	using function_type = std::function<type>;
	typedef Ret (*pointer)(Args...);
  
	static function_type Wrap(const std::string& funcName,CDllHelper& dll)
	{
		auto func = [funcName, &dll](Args&&... args){
			
			auto f = dll.GetFunction<type>(funcName);
			if (f == nullptr)
			{
				std::cout<< "can not find function: " << funcName <<std::endl;
			}
			else
			{
				//printf("call %s\n",funcName.c_str());
				return f(std::forward<Args>(args)...);
			}
		};
		return func;
	}
};


template <typename Ret, typename... Args>
struct MakeFun
{
	using type = Ret(Args...);
	using result = Ret;
};

}