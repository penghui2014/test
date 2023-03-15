#include<zlib.h>
#include<stdint.h>
#include<memory>
#include<errno.h>
#include<string.h>

int WriteToFile(const char* path, uint8_t* buffer, uint64_t length)
{
    FILE *fp = NULL;
    int write_length = 0;
    fp = fopen(path, "wb+");
    if(fp == NULL)
    {
        printf("open (wb+)%s failed\n",path);
        return -1;
    }
    write_length = fwrite(buffer, 1, length, fp);
    fclose(fp);
    fp = NULL;
    return write_length;
}

int ReadFromFile(const char* path, uint8_t* buffer, uint64_t length)
{
    FILE *fp = NULL;
    int read_length = 0;
    fp = fopen(path, "rb");
    if(fp == NULL)
    {
        printf("open (rb+)%s failed\n",path);
        return -1;
    }
    read_length = fread(buffer, 1, length, fp);
    fclose(fp);
    fp = NULL;
    return read_length;
}

int main(int argc,char** argv)
{
	int Iscompress = atoi(argv[1]);
	std::unique_ptr<Bytef> srcBuffer,dstBuffer;
	static const uLongf bufferlen  = 60*1024*1024;
	srcBuffer.reset(new Bytef[bufferlen]);
	dstBuffer.reset(new Bytef[bufferlen]);
	
	uLongf len = ReadFromFile(argv[2],srcBuffer.get(),bufferlen);
	if(len < 0)
	{
		printf("read error %s\n",strerror(errno));
		return -1;
	}
	uLongf dstLen = 0;
	//压缩
	if(Iscompress == 1)
	{
		dstLen = bufferlen;
		int ret = compress(dstBuffer.get(), (uLongf *)&dstLen, srcBuffer.get(), len);
		if(ret != Z_OK)
		{
			printf("compress error;%d\n",ret);
			return -1;
		}		
	}
	else
	{
		dstLen = bufferlen;
		int ret = uncompress(dstBuffer.get(), (uLongf *)&dstLen, srcBuffer.get(), len);
		if(ret != Z_OK)
		{
			printf("uncompress error:%d\n",ret);
			return -1;
		}
	}
	
	if(dstLen != WriteToFile(argv[3],dstBuffer.get(),dstLen))
	{
		printf("write error:%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

