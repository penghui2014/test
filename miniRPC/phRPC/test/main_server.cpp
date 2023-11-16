#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include "../include/rpc_server.hpp"

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

mystruct test2(mystruct t)
{
	return t;
}

 
int main()
{
	RPCServer server("test");
	EXPORT_RPC(server, test0);
	EXPORT_RPC(server, test1);
	EXPORT_RPC(server, add1);
	EXPORT_RPC(server, add);
	EXPORT_RPC(server, test2);
	
	while(1)
		sleep(1);
	return 0;
}
