#pragma once

#include <functional>

namespace phRPC
{
	
template <typename T> struct function_traits;

template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)> 
{
	using funT = Ret(Args...);
	using retT = Ret;
	using pointer = Ret(*)(Args...);
	using tupleT = std::tuple<Args...>;
};

template <typename Ret> 
struct function_traits<Ret()> 
{
	using funT = Ret();
	using retT = Ret;
	using pointer = Ret(*)();
	using tupleT = std::tuple<>;
};

template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : function_traits<Ret(Args...)> {};

template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>>
    : function_traits<Ret(Args...)> {};

template <typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...)>
    : function_traits<ReturnType(Args...)> {};

template <typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
    : function_traits<ReturnType(Args...)> {};


	
}	
