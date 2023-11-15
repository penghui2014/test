#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include "rpc_server.hpp"
#include "rpc_client.hpp"

using namespace std;
using namespace phRPC;

struct test
{
	int in;
	char ch;
	test(int i,char c):in(i),ch(c)
	{
		
	}
};


 
void test0(int a,long b)
{
	cout<<a<<b<<endl;
}

int add(int a,int b)
{
	return a+b;
}

string add1(string a,string b)
{
	return a + b;
}

string test1()
{
	return string("test");
}


 
int main()
{
	RPCServer server("test");
	RPCClient client("test");
	
	//test t(1,'D');
	server.Register(test0,"test0");
	server.Register(test1, "test1");
	server.Register(add1, "add1");
	server.Register(add, "add");
	//Register([](string str){cout<<str<<endl;});
	string t1 = client.Call<string()>("test1");
	cout<<"test1:"<<t1<<endl;
	cout<<"rpc_err:"<<rpc_err<<endl;
	/*
	client.Call<void(int,long)>("test0",1,9);
	
	int ret = client.Call<int(int,int)>("add",1,3);
	cout<<"add:"<<ret<<endl;
	*/
	string st = client.Call<string(string,string)>("add1","12","34");
	cout<<"add1:"<<st<<endl;
	cout<<"rpc_err:"<<rpc_err<<endl;
	
	sleep(1);
	return 0;
}
