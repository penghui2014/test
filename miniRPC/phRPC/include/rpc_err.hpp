#pragma once

#define CREATE_RPC_ERR() namespace phRPC{__thread int rpc_err = 0;}
namespace phRPC
{
	
enum RPC_ERR
{
	RPC_SUCCESS = 0,
	RPC_CONNECT_ERR = -2,
	RPC_TIMEOUT = -3,
	RPC_NO_FUNCTION = -4,
	RPC_INVALID_ARGS = -5,
	RPC_INVALID_RETURN = -6,
};

extern __thread int rpc_err;


}

