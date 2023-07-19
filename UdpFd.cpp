#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <mutex>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "RM_log.h"

#define UNIX_SOCKET_PATH "/tmp/unix_s"
#define CLIENT_SOCKET_PATH "/tmp/unix_c"

typedef struct sockaddr    TSockAddr;
typedef struct sockaddr_un TSockAddrUn;
typedef struct sockaddr_in TSockAddrIn;

static uint64_t GetSystemTickCount(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &tp);
	return (tp.tv_sec * 1000LL + tp.tv_nsec / 1000000);
}

static int InitSocket(const char* path)
{
	TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));

	unadr.sun_family = AF_LOCAL;
	strcpy(unadr.sun_path, path);

	/* 创建本地socket */
	int sockFd = socket(AF_LOCAL, SOCK_DGRAM, 0);//数据包方式
	if ( sockFd <= 0)
	{
	    perror("CUdpClient:: socket error");
		LOG_ERROR("create unix socket:%s fd:%d failed:%s\n",path,sockFd,strerror(errno));
	    return -1;
	}
	//禁止子进程继承fd
	fcntl(sockFd, F_SETFD, FD_CLOEXEC);
	
	unlink(path);

	int iRet = bind(sockFd,(TSockAddr*)&unadr, sizeof(TSockAddr));
	if (iRet != 0)
	{
	    perror("bind error");
		LOG_ERROR("bind unix socket:%s fd:%d failed:%s\n",path,sockFd,strerror(errno));
		close(sockFd);
		return -1;
	}
	LOG_CRITICAL("init unix socket:%s fd:%d successed\n",path,sockFd);
    return sockFd;
}

static int readable_timeo(int fd, int millsec)
{
	fd_set rset;
	struct timeval	tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	
	tv.tv_sec = millsec/1000;
	tv.tv_usec= millsec%1000;

	return(select(fd+1, &rset, NULL, NULL, &tv));
	/*> 0 有文件描述符可读， =0 超时 < 0 无描述符可读*/
}

static int sendFd(int fd, void* sndBuf, int sndLen)
{
	static int sockFd = -1;
	if(sockFd < 0)
	{
		#if 0
		char tmpPath[] = "/tmp/unix_XXXXXX";
		char *tmpName = mktemp(tmpPath);
		if(tmpName == NULL)
		{
			perror("mktemp error");
			LOG_ERROR("mktemp error:%s\n",strerror(errno));
			return -1;
		}
		//printf("tmpName:%s\n",tmpName);
		sockFd = InitSocket(tmpName);
		#endif
		sockFd = InitSocket(CLIENT_SOCKET_PATH);
		if(sockFd < 0)
		{
			return -1;
		}
	}
	TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));
	
	unadr.sun_family = AF_LOCAL;
	strcpy(unadr.sun_path, UNIX_SOCKET_PATH);
	//return sendto(sockFd, data, sizeof(data), 0, (TSockAddr *)&unadr, sizeof(TSockAddrUn));
	
	struct msghdr hdr;
	bzero(&hdr,sizeof(hdr));
	hdr.msg_name = (TSockAddr *)&unadr;
	hdr.msg_namelen = sizeof(TSockAddrUn);
	struct iovec iov;
	iov.iov_base = sndBuf;
	iov.iov_len = sndLen;
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;
	hdr.msg_control = NULL;
	hdr.msg_controllen = 0;
	if(fd > 0)
	{
		char control_un[CMSG_SPACE(sizeof(int))] = {0};
		hdr.msg_control = control_un;
		hdr.msg_controllen = sizeof(control_un);

		struct cmsghdr *pcmsg = CMSG_FIRSTHDR(&hdr);
		pcmsg->cmsg_len = CMSG_LEN(sizeof(int));
		pcmsg->cmsg_level = SOL_SOCKET;
		pcmsg->cmsg_type = SCM_RIGHTS;
		int* fdPtr = (int*)CMSG_DATA(pcmsg);
		*fdPtr = fd;
		//printf("msg.msg_controllen:%d pcmsg->cmsg_len:%d\n",hdr.msg_controllen,pcmsg->cmsg_len);
	}
	int ret = sendmsg(sockFd, &hdr, MSG_NOSIGNAL);
	if(ret <= 0)
	{
		//printf("sendMsg ERROR:%s\n",strerror(errno));
		//LOG_ERROR("sendMsg ERROR:%s\n",strerror(errno));
	}
	return ret;
}

static int ReciveFd(int* fd,void* recvBuf,int bufLen,int timeOut)
{
	static int sockFd = -1;
	*fd = -1;
	if(sockFd < 0)
	{	
		sockFd = InitSocket(UNIX_SOCKET_PATH);
		if(sockFd < 0)
		{
			return -1;
		}
	}
	int nRead = readable_timeo(sockFd, timeOut);
	if ( nRead <= 0 )
	{
		//printf("read time out!\n");
		LOG_DEBUG("read time out\n");
		return 0;
	}
	TSockAddrUn unClientaddr;
	socklen_t socklen  = sizeof(TSockAddrUn);

	//nRead = recvfrom(sockFd, recvBuf, 32, 0, (TSockAddr *)&unClientaddr, &socklen);
	struct msghdr msg;
	memset(&msg,0,sizeof(struct msghdr));
	struct iovec iov[1];
	//Control_u control_un;

	char control[CMSG_SPACE(sizeof(int))];

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	
	iov[0].iov_base = recvBuf;
	iov[0].iov_len = bufLen;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	
	//printf("msg.msg_controllen:%d sizeof(control_un):%d\n",msg.msg_controllen,sizeof(control_un));
	
	int s32RecvLen = recvmsg(sockFd , &msg, 0);
	
	if ( s32RecvLen <= 0 )
	{
		if ( (EAGAIN == errno) || (EINTR == errno))
		{
			return 0;   //接收连接超时
		}
		LOG_ERROR("read error::%s\n",strerror(errno));
		perror("read error:");
		return -1;
	}
	
	struct cmsghdr* pcmsg = CMSG_FIRSTHDR(&msg);
    if(pcmsg != NULL) 
    {
		if(pcmsg->cmsg_type == SCM_RIGHTS && pcmsg->cmsg_level == SOL_SOCKET && pcmsg->cmsg_len == CMSG_LEN(sizeof(int)) )
		{
			int* pFds = (int*)CMSG_DATA(pcmsg);
			*fd = *pFds;
		}
        else
        {
            printf("pcmsg is error type:%d level:%d len:%ld\n",pcmsg->cmsg_type, pcmsg->cmsg_level, pcmsg->cmsg_len);
			LOG_ERROR("pcmsg is error type:%d level:%d len:%ld\n",pcmsg->cmsg_type, pcmsg->cmsg_level, pcmsg->cmsg_len);
        }
    }
	else
	{
		//printf("ERROR pcmsg is NULL\n");
		LOG_DEBUG("pcmsg is NULL\n");
	}
	
	//printf("recive %d:%s fd:%d\n",nRead,recvBuf,fd);
	
	return s32RecvLen;
}

static int InitFdReader(int chn)
{
    char path[64] = {0};
    sprintf(path,"/tmp/r_%d",chn);
    return InitSocket(path);
}


static int InitFdWriter(int chn)
{
    char path[64] = {0};
    sprintf(path,"/tmp/w_%d",chn);
    return InitSocket(path);
}


static int FdRead(int chn,int* fd,void* readBuf,int bufLen,int timeOut)
{
    static int sockFds[8] = {-1};
    if(chn <0 || chn >= 8)
    {
        return -1;
    }
    
	*fd = -1;
	if(sockFds[chn] <= 0)
	{	
		sockFds[chn] = InitFdReader(chn);
		if(sockFds[chn] <= 0)
		{
			return -1;
		}

	}
	int sockFd = sockFds[chn];

	*fd = -1;
	if(sockFd <= 0)
	{	
	    LOG_DEBUG("ERROR fd\n");
		return -1;
	}
	int nRead = readable_timeo(sockFd, timeOut);
	if ( nRead <= 0 )
	{
		//printf("read time out!\n");
		LOG_DEBUG("read time out\n");
		return 0;
	}
	TSockAddrUn unClientaddr;
	socklen_t socklen  = sizeof(TSockAddrUn);

	//nRead = recvfrom(sockFd, recvBuf, 32, 0, (TSockAddr *)&unClientaddr, &socklen);
	struct msghdr msg;
	memset(&msg,0,sizeof(struct msghdr));
	struct iovec iov[1];
	//Control_u control_un;

	char control[CMSG_SPACE(sizeof(int))];

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	
	iov[0].iov_base = readBuf;
	iov[0].iov_len = bufLen;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	
	//printf("msg.msg_controllen:%d sizeof(control_un):%d\n",msg.msg_controllen,sizeof(control_un));
	int s32RecvLen = 0;
	do
	{
		s32RecvLen = recvmsg(sockFd , &msg, 0);
		
		if ( s32RecvLen <= 0 )
		{
			if ( (EAGAIN == errno) || (EINTR == errno))
			{
				return 0;   //接收连接超时
			}
			LOG_ERROR("read error::%s\n",strerror(errno));
			return -1;
		}
		
		struct cmsghdr* pcmsg = CMSG_FIRSTHDR(&msg);
	    if(pcmsg != NULL) 
	    {
			if(pcmsg->cmsg_type == SCM_RIGHTS && pcmsg->cmsg_level == SOL_SOCKET && pcmsg->cmsg_len == CMSG_LEN(sizeof(int)) )
			{
				int* pFds = (int*)CMSG_DATA(pcmsg);
				*fd = *pFds;
			}
	        else
	        {
	            printf("pcmsg is error type:%d level:%d len:%ld\n",pcmsg->cmsg_type, pcmsg->cmsg_level, pcmsg->cmsg_len);
				LOG_ERROR("pcmsg is error type:%d level:%d len:%ld\n",pcmsg->cmsg_type, pcmsg->cmsg_level, pcmsg->cmsg_len);
	        }
	    }
		else
		{
			//printf("ERROR pcmsg is NULL\n");
			LOG_DEBUG("pcmsg is NULL\n");
		}
	}while(s32RecvLen > 0)
	//printf("recive %d:%s fd:%d\n",nRead,recvBuf,fd);
	
	return s32RecvLen;
}

static int FdWrite(int chn,int fd, void* writeBuf, int writeLen)
{
    static int sockFds[8] = {-1};
    if(chn <0 || chn >= 8)
    {
        return -1;
    }

	if(sockFds[chn] <= 0)
	{	
		sockFds[chn] = InitFdWriter(chn);
		if(sockFds[chn] <= 0)
		{
			return -1;
		}
	}
	
	int sockFd = sockFds[chn];
	
	if(sockFd <= 0)
	{
	    LOG_ERROR("ERROR fd\n");
		return -1;
	}
	
	TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));
	char path[64] = {0};
    sprintf(path,"/tmp/r_%d",chn);
	unadr.sun_family = AF_LOCAL;
	strcpy(unadr.sun_path, path);
	//return sendto(sockFd, data, sizeof(data), 0, (TSockAddr *)&unadr, sizeof(TSockAddrUn));
	
	struct msghdr hdr;
	bzero(&hdr,sizeof(hdr));
	hdr.msg_name = (TSockAddr *)&unadr;
	hdr.msg_namelen = sizeof(TSockAddrUn);
	struct iovec iov;
	iov.iov_base = writeBuf;
	iov.iov_len = writeLen;
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;
	hdr.msg_control = NULL;
	hdr.msg_controllen = 0;
	if(fd > 0)
	{
		char control_un[CMSG_SPACE(sizeof(int))] = {0};
		hdr.msg_control = control_un;
		hdr.msg_controllen = sizeof(control_un);

		struct cmsghdr *pcmsg = CMSG_FIRSTHDR(&hdr);
		pcmsg->cmsg_len = CMSG_LEN(sizeof(int));
		pcmsg->cmsg_level = SOL_SOCKET;
		pcmsg->cmsg_type = SCM_RIGHTS;
		int* fdPtr = (int*)CMSG_DATA(pcmsg);
		*fdPtr = fd;
		//printf("msg.msg_controllen:%d pcmsg->cmsg_len:%d\n",hdr.msg_controllen,pcmsg->cmsg_len);
	}
	int ret = sendmsg(sockFd, &hdr, MSG_NOSIGNAL|MSG_DONTWAIT);
	if(ret <= 0)
	{
		//printf("sendMsg ERROR:%s\n",strerror(errno));
		LOG_ERROR("sendMsg ERROR:%s\n",strerror(errno));
	}
	return ret;
}

int main(int argc,char** argv)
{
	if(argc == 1)
	{
		LOG_ERROR("start writer\n");
		int count[6] = {0};
		int fps[6] = {0};
		uint64_t last[6] = {0};
		while(1)
		{
		    char buffer[64] = {0};
			int i = 1;
		    //for(int i = 0; i < 6;i++)
		    {
				count[i]++;
		        sprintf(buffer,"hello this is chn:%d cnt:%d",i,count[i]);
    		    uint64_t start = GetSystemTickCount();
    		    FdWrite(i,count[i]%3,buffer,strlen(buffer));
    		    LOG_ERROR("chn:%d cost :%lu count:%d\n",i,GetSystemTickCount()-start,count[i]);
		    }
		    usleep(500*1000);
		    //LOG_ERROR("cost8888 :%llu\n",GetSystemTickCount()-start);
		}
	}
	else
	{
        LOG_ERROR("start reader\n");
        int fd;
        while(1)
        {
            char buffer[1024] = {0};
            int len = FdRead(atoi(argv[1]),&fd, buffer, 1024, 1000);
            if(len > 0) 
			{
				LOG_ERROR("%s fd:%d\n",buffer,fd);
				close(fd);
			}
    		sleep(1);
        }
	}
	
}