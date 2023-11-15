#include <iostream>
#include <dlfcn.h>
#include <map>
#include <string>
#include <functional>
#include <unistd.h>
#include "../dllhelper/dll.hpp"

//using namespace miniRPC;




struct SharedFrame_t
{
	uint64_t pts{0};
	uint64_t size{0};
	uint32_t type{0};//0:rgba
	uint32_t width{0};
	uint32_t height{0};
	
	int 	fd{-1};
	void*    data;
};

bool InitPreview(long chn);
//小于0失败，大于等于0成功，成功后需要调用PreviewRelease接口释放帧
int PreviewRequest(int chn,SharedFrame_t* frame);
void PreviewRelease(int chn);
bool InitAIReader(int chn);
int AIRequest(int chn,SharedFrame_t* frame);
bool AIRelease(int chn);



static miniRPC::CDllHelper dll("/usr/local/lib/libsharedvideo.so");

//void AIRelease(int);
/*
using AIRelease_type = decltype(AIRelease);
using AIRelease_ret = WrapperFun<AIRelease_type>::return_type;

static const std::string AIRelease_name = MangleCpp<AIRelease_type>("AIRelease");;

template<typename ...Args>
AIRelease_ret AIRelease(Args&&... arg)
{
	static auto fun = miniRPC::WrapperFun<AIRelease_type>::Wrap(AIRelease_name,dll);
	return fun(std::forward<Args>(arg)...);
}*/


//IMPORT_FUNCPP(dll,AIRelease);

//IMPORT_CALLCPP(dll,bool,InitAIReader,int);



IMPORT_FUNCPP(dll,InitAIReader);

IMPORT_FUNCPP(dll,AIRelease);
//IMPORT_FUNCPP(dll,InitAVEncoder);

//CREATE_FUNCPP(dll,void,AIRelease,int);
IMPORT_CALLCPP(dll,int,PreviewRequest,int,SharedFrame_t*);
IMPORT_CALLCPP(dll,void,PreviewRelease,int);



int main()
{
	//dll.Load();
	int a = 0;
	
	//CALL_FUNC(AIRelease,0);
	//CALL_FUNCPP(AIRelease,0);


	bool ret = CALL_FUNCPP(dll,InitPreview,0);
	printf("ret:%d\n",ret);
	//CALL_AIRelease(0);
	//InitAVEncoder(0);
	InitAIReader<>(0);
	while(1)
	{
		SharedFrame_t frame;
		if(0 <= PreviewRequest<>(0,&frame))
		{
			//std::cout<<"dddddddddddddddddd\n";
			PreviewRelease<>(0);
		}
		//PreviewRelease(0);
		usleep(20*1000);
	}
	
	//AIRequest(0,nullptr);
	AIRelease(0);
	
	//auto f = help.Wrap<int(int)>("_Z9AIReleasei");
	//CDllHelper::function_traits<int(int)> f;

	//FunctionInfo<decltype(AIRelease)>::remoteCall("_Z9AIReleasei",0);
	return 0;
}