#pragma once

#include <string>
#include <functional>
#include "dllHelp.hpp"

namespace miniRPC
{

template<typename Ret,typename ...Args>
struct WrapperArg
{
	using function_type = Ret(Args...);
	static auto Wrap(const std::string& funcName,CDllHelper& dll)
	->std::function<function_type>
	{
		auto func = [funcName,&dll](Args&&... args)
		{
			auto f = dll.GetFunction<function_type>(funcName);
			if (f == nullptr)
			{
				std::cout<< "can not find function: " << funcName <<std::endl;
			}
			else
			{
				printf("call %s\n",funcName.c_str());
				return f(std::forward<Args>(args)...);
			}
		};
		return func;
	}
};

template <typename T> 
struct WrapperFun;

template <typename Ret, typename... Args>
struct WrapperFun<Ret(Args...)> 
{
	typedef Ret function_type(Args...);
	typedef Ret return_type;

	using stl_function_type = std::function<function_type>;
	typedef Ret (*pointer)(Args...);
  
	static stl_function_type Wrap(const std::string& funcName,CDllHelper& dll)
	{
		auto func = [funcName, &dll](Args&&... args){
			
			auto f = dll.GetFunction<function_type>(funcName);
			if (f == nullptr)
			{
				std::cout<< "can not find function: " << funcName <<std::endl;
			}
			else
			{
				printf("call %s\n",funcName.c_str());
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