#pragma once

#include "include/wrapper.hpp"
#include "include/dllHelp.hpp"
#include "include/mangle.hpp"


#define STR1(R) #R

#define CALL_FUNC(dll,fun,...) \
dll.Invoke<decltype(fun)>(STR1(fun),##__VA_ARGS__);

#define CALL_FUNCPP(dll,fun,...) \
dll.Invoke<decltype(fun)>(MANGLE(fun),##__VA_ARGS__);

/*
#define IMPORT_FUNC(dll,fun) \
static auto CALL_##fun = miniRPC::WrapperFun<decltype(fun)>::Wrap(STR1(fun),dll);

#define IMPORT_FUNCPP(dll,fun) \
static auto CALL_##fun = miniRPC::WrapperFun<decltype(fun)>::Wrap(MANGLE(fun),dll);

#define CREATE_FUNC(dll,ret,fun,...) \
static auto fun = miniRPC::WrapperArg<ret,__VA_ARGS__>::Wrap(STR1(fun),dll);

#define CREATE_FUNCPP(dll,ret,fun,...) \
static auto fun = miniRPC::WrapperArg<ret,__VA_ARGS__>::Wrap(MANGLEARG(fun,##__VA_ARGS__),dll);
*/


#define IMPORT_CALLC(dll,ret,fun,...) \
using fun_##type = MakeFun<ret,__VA_ARGS__>::type; \
using fun_##ret = MakeFun<ret,__VA_ARGS__>::result; \
static const std::string fun_##name = STR1(fun); \
template<typename ...Args> \
static fun_##ret fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::WrapperFun<fun_##type>::Wrap(fun_##name,dll); \
	return s_##fun(std::forward<Args>(arg)...);\
}

#define IMPORT_CALLCPP(dll,ret,fun,...) \
using fun_##type = MakeFun<ret,__VA_ARGS__>::type; \
using fun_##ret = MakeFun<ret,__VA_ARGS__>::result; \
static const std::string fun_##name = MangleCpp<fun_##type>(STR1(fun)); \
template<typename ...Args> \
static ret fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::WrapperFun<fun_##type>::Wrap(fun_##name,dll); \
	return s_##fun(std::forward<Args>(arg)...);\
}


#define IMPORT_FUNC(dll,fun) \
using fun_##type = decltype(fun); \
using fun_##ret = WrapperFun<fun_##type>::return_type; \
static const std::string fun_##name = STR1(fun);\
template<typename ...Args> \
static fun_##ret fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::WrapperFun<fun_##type>::Wrap(fun_##name,dll);\
	return s_##fun(std::forward<Args>(arg)...);\
}


#define IMPORT_FUNCPP(dll,fun) \
using fun_##type = decltype(fun); \
using fun_##ret = WrapperFun<fun_##type>::return_type; \
static const std::string fun_##name = MANGLE(fun);\
template<typename ...Args> \
static fun_##ret fun(Args&&... arg) \
{\
	static auto s_##fun = miniRPC::WrapperFun<fun_##type>::Wrap(fun_##name,dll);\
	return s_##fun(std::forward<Args>(arg)...);\
}

