#pragma once

#include "include/wrapper.hpp"
#include "include/dllHelp.hpp"
#include "include/mangle.hpp"


#define STR1(R) #R

#define CALL_FUNC(dll,fun,...) \
dll.InvokeC<decltype(fun)>(STR1(fun),##__VA_ARGS__);

#define CALL_FUNCPP(dll,fun,...) \
dll.InvokeCpp<decltype(fun)>(STR1(fun),##__VA_ARGS__);



#define IMPORT_CALLC(dll,ret,fun,...) \
using type_##fun = miniRPC::MakeFun<ret,__VA_ARGS__>::type; \
using ret_##fun = miniRPC::MakeFun<ret,__VA_ARGS__>::result; \
static const std::string name_##fun = STR1(fun); \
template<typename ...Args> \
static ret_##fun fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::Wrapper<type_##fun>::Wrap(name_##fun,dll); \
	return s_##fun(std::forward<Args>(arg)...);\
}

#define IMPORT_CALLCPP(dll,ret,fun,...) \
using type_##fun = miniRPC::MakeFun<ret,__VA_ARGS__>::type; \
using ret_##fun = miniRPC::MakeFun<ret,__VA_ARGS__>::result; \
static const std::string name_##fun = miniRPC::MangleCpp<type_##fun>(STR1(fun)); \
template<typename ...Args> \
static ret_##fun fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::Wrapper<type_##fun>::Wrap(name_##fun,dll); \
	return s_##fun(std::forward<Args>(arg)...);\
}


#define IMPORT_FUNC(dll,fun) \
using type_##fun = decltype(fun); \
using ret_##fun = miniRPC::Wrapper<type_##fun>::result; \
static const std::string name_##fun = STR1(fun);\
template<typename ...Args> \
static ret_##fun fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::Wrapper<type_##fun>::Wrap(name_##fun,dll);\
	return s_##fun(std::forward<Args>(arg)...);\
}


#define IMPORT_FUNCPP(dll,fun) \
using type_##fun = decltype(fun); \
using ret_##fun = miniRPC::Wrapper<type_##fun>::result; \
static const std::string name_##fun = miniRPC::MangleCpp<type_##fun>(STR1(fun));\
template<typename ...Args> \
ret_##fun fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::Wrapper<type_##fun>::Wrap(name_##fun,dll);\
	return s_##fun(std::forward<Args>(arg)...);\
}

