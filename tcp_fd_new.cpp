//#include <sys/syscall.h>
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
#include "RM_log.h"


#define DOMAN "/tmp/writer"
#define MAXCLINE 16
#define MAX_CHN 6


typedef struct sockaddr    TSockAddr;
typedef struct sockaddr_un TSockAddrUn;
typedef struct sockaddr_in TSockAddrIn;
//#define LOG_ERROR printf
//#define LOG_CRITICAL printf
//#define LOG_DEBUG printf

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


static uint64_t GetSystemTickCount(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &tp);
	return (tp.tv_sec * 1000LL + tp.tv_nsec / 1000000);
}

static int	InitClient(const char* path)
{	
	int ret = -1;
	
	struct sockaddr_un servaddr;  //IPC
	  
	int fd = socket(AF_UNIX, SOCK_STREAM, 0) ;
	if(fd < 0) 
	{
		LOG_ERROR("socket failed.\n") ;
		return  -1;
	}

	fcntl(fd, F_SETFD, FD_CLOEXEC);
	int sendBuffSize = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &sendBuffSize, sizeof(sendBuffSize));
	if(ret < 0)
	{
		LOG_ERROR("socket set sendbuf opt fail\n");
		return -1;
	}
	TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));

	unadr.sun_family = AF_UNIX;
	strcpy(unadr.sun_path, path);
	unlink(path);

	int iRet = bind(fd,(TSockAddr*)&unadr, sizeof(TSockAddr));
	if (iRet != 0)
	{
	    perror("bind error");
		LOG_ERROR("bind unix socket:%s fd:%d failed:%s\n",path,fd,strerror(errno));
		close(fd);
		return -1;
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	strcpy (servaddr.sun_path, DOMAN);
	
	ret = connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	
	if(ret < 0) 
	{
		LOG_ERROR("connect failed. domain name:%s %s\n",DOMAN,strerror(errno)) ;
		close(fd);
		fd = -1;
		return ret;
	}
	//CleanRecvBuffer(m_nFd);
	LOG_ERROR("connect server success fd:%d\n",fd);
	return fd;
}

static int InitServer()
{
	int ret = -1;
	struct sockaddr_un servaddr;
	
	int fd  =  socket (AF_UNIX , SOCK_STREAM , 0 ) ;
	if(fd < 0) {
		LOG_ERROR("%s socket failed.\n",DOMAN) ;
		return  -1;
	}

	unlink(DOMAN);
	int sendBuffSize = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &sendBuffSize, sizeof(sendBuffSize));
	if(ret < 0)
	{
		LOG_ERROR("socket set sendbuf opt fail\n");
		return -1;
	}
	bzero (&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	strcpy (servaddr.sun_path , DOMAN);

	ret  =  bind (fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if(ret < 0) {
		LOG_ERROR("%s bind failed. errno = %d.\n" , DOMAN, errno ) ;
		close(fd);
		return  - 1 ;
	}

	listen(fd, 20);
	//std::string cmd = "chmod 777 " + std::string(m_nDomanPath);
	//system(cmd.c_str());
	
	LOG_ERROR("%s start listen:%d\n", DOMAN, fd);
	
	return fd;
}


static std::vector<int> g_writer[MAX_CHN];
static std::mutex g_mutex[MAX_CHN];

static void* ServerThread(void*)
{
    int fd = 0;
    struct sockaddr_un cliaddr;
    unsigned int clilen = sizeof(cliaddr);
	while(1)
	{
	    if(0 >= fd)
	    {
            fd = InitServer();
	    }
	    
		if(0 >= fd)
		{
			sleep(1);
			continue;
		}
        int canread = readable_timeo(fd, 1000);
    	if ( canread <= 0 )
    	{
    		//LOG_ERROR("read time out!\n");
    		//LOG_ERROR("read time out\n");
    		continue;
    	}
    	
		int new_fd = accept(fd, (struct sockaddr*)&cliaddr , (socklen_t *)&clilen);

		if(new_fd <= 0)
		{
			LOG_ERROR("accept error [%s]\n",strerror(errno));
			close(fd);
			fd = 0;
			continue;
		}
        else
        {
            LOG_ERROR("client:%s\n",cliaddr.sun_path);
            int chn = cliaddr.sun_path[5] - '0';
            if(chn < 0 || chn >= MAX_CHN)
            {
                LOG_ERROR("channel error:%d\n",chn);
                close(new_fd);
                continue;
            }
            std::lock_guard<std::mutex> lock(g_mutex[chn]);
            if(g_writer[chn].size() > MAXCLINE)
            {
                LOG_ERROR("g_writer %d full\n",chn);
            }
            else
            {
                g_writer[chn].push_back(new_fd);
                LOG_ERROR("accept :%d size:%d chn:%d\n",new_fd,(int)g_writer[chn].size(),chn);
            }
        }				
	}
	
	LOG_ERROR("Server thread exit\n");
	for(int i = 0;i < MAX_CHN; i++)
	{
    	std::lock_guard<std::mutex> lock(g_mutex[i]);
    	while(g_writer[i].size())
    	{
    		if(g_writer[i][0] != 0)
    		{
    			close(g_writer[i][0]);
    		}
    		g_writer[i].erase(g_writer[i].begin());
    	}
	}

	close(fd);

	fd = 0;
	return nullptr;
}


int FdWrite(int chn,int fd, void* writeBuf, int writeLen)
{
    if(chn < 0 || chn >= MAX_CHN)
    {
        LOG_ERROR("channel ERROR:%d\n",chn);
        return -1;
    }
	static bool bInit = false;
	if(!bInit)
	{
	    bInit = true;
	    pthread_t tmp;
        pthread_create(&tmp,nullptr,&ServerThread,nullptr);       
	}
	TSockAddrUn unadr;
	bzero(&unadr,sizeof(unadr));
	
	//unadr.sun_family = AF_LOCAL;
	//strcpy(unadr.sun_path, UNIX_SOCKET_PATH);
	//return sendto(sockFd, data, sizeof(data), 0, (TSockAddr *)&unadr, sizeof(TSockAddrUn));
	
	struct msghdr hdr;
	bzero(&hdr,sizeof(hdr));
	//hdr.msg_name = (TSockAddr *)&unadr;
	//hdr.msg_namelen = sizeof(TSockAddrUn);
	hdr.msg_name = nullptr;
	hdr.msg_namelen = 0;
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
		//LOG_ERROR("msg.msg_controllen:%d pcmsg->cmsg_len:%d\n",hdr.msg_controllen,pcmsg->cmsg_len);
	}
	std::lock_guard<std::mutex> lock(g_mutex[chn]);
	for(int i = 0;i < g_writer[chn].size();)
	{
    	int ret = sendmsg(g_writer[chn][i], &hdr, MSG_NOSIGNAL|MSG_DONTWAIT);
    	if(ret <= 0)
    	{
    		LOG_ERROR("sendMsg ERROR:%s close %d\n",strerror(errno),g_writer[chn][i]);
    		close(g_writer[chn][i]);
    		g_writer[chn].erase(g_writer[chn].begin()+i);
    		//LOG_ERROR("sendMsg ERROR:%s\n",strerror(errno));
    	}
    	else
    	{
    	    i++;
        }
    }
	return 0;
}

int FdRead(int chn,int* fd,void* readBuf,int bufLen,int timeOut)
{
	static int s_sockFd[MAX_CHN] = {0};
	if(chn < 0 || chn >= MAX_CHN)
	{
        LOG_ERROR("chn error:%d\n",chn);
        return -1;
	}
	
	if(s_sockFd[chn] <= 0)
	{	
	    char tmpPath[32] = {0};
	    sprintf(tmpPath,"/tmp/%d_XXXXXX",chn);
		char *tmpName = mktemp(tmpPath);
		if(tmpName == NULL)
		{
			perror("mktemp error");
			LOG_ERROR("mktemp error:%s\n",strerror(errno));
			return -1;
		}
		s_sockFd[chn] = InitClient(tmpName);
		if(s_sockFd[chn] <= 0)
		{
		    usleep(timeOut*1000);
			return -1;
		}
	}
	#if 0
	int nRead = readable_timeo(s_sockFd[chn], timeOut);
	if ( nRead <= 0 )
	{
		//LOG_ERROR("read time out!\n");
		LOG_DEBUG("read time out\n");
		return 0;
	}
	#endif
	int lastRecvLen = 0;
	int s32RecvLen = 0;
	int recvCnt = 0;
	int recvFd = -1;
	do
	{
		recvCnt++;
		
		TSockAddrUn unClientaddr;
		socklen_t socklen  = sizeof(TSockAddrUn);

		struct msghdr msg;
		memset(&msg,0,sizeof(struct msghdr));
		struct iovec iov[1];

		char control[CMSG_SPACE(sizeof(int))];
		
	    TSockAddrUn unadr;
		bzero(&unadr,sizeof(unadr));
		
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		
		iov[0].iov_base = readBuf;
		iov[0].iov_len = bufLen;
		msg.msg_iov = iov;
		msg.msg_iovlen = 1;
		
		msg.msg_control = control;
		msg.msg_controllen = sizeof(control);
		
		//LOG_ERROR("msg.msg_controllen:%d sizeof(control_un):%d\n",msg.msg_controllen,sizeof(control_un));
		
		s32RecvLen = recvmsg(s_sockFd[chn] , &msg, MSG_DONTWAIT);
		//LOG_ERROR("s32RecvLen%d\n", s32RecvLen);
		if ( s32RecvLen <= 0 )
		{
			if ( (EAGAIN == errno) || (EINTR == errno))
			{
				break;   //接收连接超时
			}
			else
			{
				LOG_ERROR("read error::%s\n",strerror(errno));
				perror("read error:");
				close(s_sockFd[chn]);
				s_sockFd[chn] = 0;
				lastRecvLen = -1;
				break;
			}
		}

		lastRecvLen = s32RecvLen;
		
		struct cmsghdr* pcmsg = CMSG_FIRSTHDR(&msg);
	    if(pcmsg != NULL) 
	    {
			if(pcmsg->cmsg_type == SCM_RIGHTS && pcmsg->cmsg_level == SOL_SOCKET && pcmsg->cmsg_len == CMSG_LEN(sizeof(int)) )
			{
				int* pFds = (int*)CMSG_DATA(pcmsg);
				
				if(nullptr != pFds)
				{
					if(recvFd > 0)
					{
						close(recvFd);
					}
				    recvFd = *pFds;
				}
			}
	        else
	        {
	            LOG_ERROR("pcmsg is error type:%d level:%d len:%ld\n",pcmsg->cmsg_type, pcmsg->cmsg_level, pcmsg->cmsg_len);
				LOG_ERROR("pcmsg is error type:%d level:%d len:%ld\n",pcmsg->cmsg_type, pcmsg->cmsg_level, pcmsg->cmsg_len);
				break;
	        }
	    }
		else
		{
			//LOG_ERROR("ERROR pcmsg is NULL\n");
			LOG_DEBUG("pcmsg is NULL\n");
			break;
		}
	}while(0 < s32RecvLen && 256 > recvCnt);
	//LOG_ERROR("recive %d:%s fd:%d\n",nRead,recvBuf,fd);
	if(lastRecvLen <= 0)
	{
		usleep(10*1000);
	}
	
	return lastRecvLen;
}

#if 0
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
    		    FdWrite(i,count[i]%3 + 1,buffer,sizeof(buffer));
    		    LOG_ERROR("chn:%d cost :%lu count:%d\n",i,GetSystemTickCount()-start,count[i]);
		    }
		    usleep(50*1000);
		    //LOG_ERROR("cost8888 :%llu\n",GetSystemTickCount()-start);
		}
	}
	else
	{
        LOG_ERROR("start reader\n");
        int fd;
        while(1)
        {
            char buffer[64] = {0};
            int len = FdRead(atoi(argv[1]),&fd, buffer, 64, 1000);
            if(len > 0) 
			{
				LOG_ERROR("%s fd:%d\n",buffer,fd);
				close(fd);
			}
    		sleep(1);
        }
	}
	
}
#endif
