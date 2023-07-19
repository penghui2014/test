#ifndef SHARE_FD_H__
#define SHARE_FD_H__

int sendFd(int fd, void* sndBuf, int sndLen);

//timeout:milli second
int ReciveFd(int* fd,void* recvBuf,int bufLen,int timeOut);
int InitFdReader(int chn);
int InitFdWriter(int chn);
int FdRead(int handle,int* fd,void* readBuf,int bufLen,int timeOut);
int FdWrite(int handle,int fd, void* writeBuf, int writeLen);
#endif