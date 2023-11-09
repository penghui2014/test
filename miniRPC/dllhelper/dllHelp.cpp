#include <iostream>
#include <dlfcn.h>
#include <map>
#include <string>
#include <functional>
#include "dllhelper/dll.hpp"

using namespace miniRPC;





static CDllHelper dll("/usr/local/lib/libsharedvideo.so");

void AIRelease(int);



/*
using AIRelease_type = decltype(AIRelease);
using AIRelease_ret = WrapperFun<AIRelease_type>::return_type;

static const std::string AIRelease_name = MANGLE(AIRelease);

template<typename ...Args>
AIRelease_ret AIRelease(Args&&... arg)
{
	static auto fun = miniRPC::WrapperFun<AIRelease_type>::Wrap(AIRelease_name,dll);
	return fun(std::forward<Args>(arg)...);
}*/

//IMPORT_FUNCPP(dll,AIRelease);

IMPORT_CALLCPP(dll,void,AIRelease,int);

//IMPORT_FUNCPP(AIRelease);
//CREATE_FUNCPP(dll,void,AIRelease,int);

int main()
{
	//dll.Load();
	int a = 0;
	//CALL_FUNC(AIRelease,0);
	//CALL_FUNCPP(AIRelease,0);

	//CALL_FUNCPP(AIRelease,0);
	//CALL_AIRelease(0);
	AIRelease(1.2);
	
	//AIRelease(0,0);
	
	//auto f = help.Wrap<int(int)>("_Z9AIReleasei");
	//CDllHelper::function_traits<int(int)> f;

	//FunctionInfo<decltype(AIRelease)>::remoteCall("_Z9AIReleasei",0);
	return 0;
}