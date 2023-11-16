#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <thread>

#include "../include/rpc_client.hpp"

CREATE_RPC_ERR();

using namespace std;
using namespace phRPC;

struct mystruct
{
	int in;
	char ch;
	mystruct(){}
	mystruct(int i,char c):in(i),ch(c)
	{
		
	}
};


 
void test0(int a,long b);

int add(int a,int b);

string add1(string a, string b);

string test1();

mystruct test2(mystruct t);

RPCClient client("test");


IMPORT_RPC(client,test0);
IMPORT_RPC(client,add);
IMPORT_RPC(client,add1);
IMPORT_RPC(client,test1);
IMPORT_RPC(client,test2);

void test(int i)
{
	while(1)
	{
		if(i == 0)
			test0<>(128,10);
		if(i == 1)
		{
			int ret = add<>(1,2);
			cout<<"add:"<<ret<<endl;
		}
		if(i == 2)
		{
			string arg1 = "fuck ";
			string arg2 = "this world";
			string ret = add1<>(arg1,arg2);
			cout<<"add1:"<<ret<<endl;
		}
		if(i == 3)
		{
			string ret = test1<>();
			cout<<"test1:"<<ret<<endl;
		}
		if(i == 4)
		{
			mystruct my;
			auto ret = test2<>(my);
			//cout<<"test2:"<<ret<<endl;
		}
		usleep(100*1000);
	}
}

int main()
{
	thread t[5];
	for(int i = 0;i < 4; i++)
	{
		t[i] = thread(test,i);
	}
	
	
	while(1)
	{
		
		usleep(40*1000);
	}

	return 0;
}
