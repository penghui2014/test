#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

struct Time
{
	int hour;
	int minu;
	int sec;
	int mSec;
};


struct CanInfo_t
{
	int sn;
	int line;
	int id;
	Time time;
	uint8_t data[8];
};


static string StrCanData(uint8_t data[8])
{
	char st[32] = {0};
	sprintf(st, "%02X %02X %02X %02X %02X %02X %02X %02X",
		data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
	
	return string(st);
}

static string StrCanTime(Time time)
{
	char st[32] = {0};
	sprintf(st,"%02d:%02d:%02d.%03d",time.hour,time.minu,time.sec,time.mSec);
	
	return string(st);
}

static uint64_t ToMSecond(Time time)
{
	return (time.hour*60*60*1000) + (time.minu*60*1000) + (time.sec*1000) + (time.mSec);
}

static CanInfo_t ParseCan(char* buffer, int line, const char* c)
{
	CanInfo_t can;
	vector<string> tak;
	can.line = line;
	
	buffer = strtok(buffer, c);

	while(buffer)
	{
		tak.push_back(buffer);
		buffer = strtok(NULL, c);
	}

	sscanf(tak[0].c_str(),"%d",&can.sn);
	sscanf(tak[2].c_str(),"%02d:%02d:%02d.%03d",&can.time.hour,&can.time.minu,&can.time.sec,&can.time.mSec);
	sscanf(tak[3].c_str(),"0x%08X",&can.id);
	sscanf(tak[7].c_str(),"%02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX ",&can.data[0],&can.data[1],&can.data[2],&can.data[3],&can.data[4],&can.data[5],&can.data[6],&can.data[7]);

	//printf("id:%08X %s [%s]\n",can.id, StrCanTime(can.time).c_str(), StrCanData(can.data).c_str());
	
	return can;
}


struct DBCValue
{
	string name;
	uint64_t value;
};

struct MiniDBC
{
#define BIT_M_TO_L(x, s, l) (((x)>>(s)) & (0xFFFFFFFFFFFFFFFF>>(64-(l))))

	const string name;
	const int id;
	const uint8_t bitStart;
	const uint8_t bitLen;
	const uint32_t interval;
	const vector<DBCValue> valueMap;

	MiniDBC(string n,int i,uint8_t start,uint8_t len, uint32_t inter, vector<DBCValue> map):
		name(n),id(i),bitStart(start),bitLen(len),interval(inter),valueMap(map)
	{
		memset(&lastTime,0,sizeof(lastTime));
		memset(&lastWriteTime,0,sizeof(lastWriteTime));
		lastValue = 0;
	}
		
	Time	lastTime;
	Time	lastWriteTime;
	uint64_t 	lastValue;
	
	uint64_t parse(uint8_t data[8])
	{	
		uint64_t tmp;
		memcpy(&tmp, data,8);
		return BIT_M_TO_L(tmp,bitStart,bitLen);
	}

};

#if 0
MiniDBC g_DBC[] = {
	{"油门值",0x210,16,8,40},
	{"模式",0x210,0,8,40,{{"自动",2},{"人工",1}}},
	{"挡位",0x210,8,8,40,{{"空挡",0},{"前进",1},{"倒挡",2}}},
	{"踏板开度", 0x142,32,8,20},
	{"电子手刹反馈",0x240,51,1,100,{{"拉起",0},{"松开",1}}},
	{"电子手刹控制",0x242,8,2,100,{{"拉起",1},{"松开",2}}},
};
#endif

vector<string> Strtok(const char* src, const char* c)
{
	vector<string> tmp;
	char* a = strtok((char*)src, c);
	while(a)
	{
		tmp.push_back(a);
		a = strtok(NULL, c);
	}
	return tmp;
}

bool ParseMiniDBCFile(const char* file, vector<MiniDBC>& dbcList)
{
	ifstream in;
	in.open(file, ios::binary);
	if(!in.is_open())
	{
		fprintf(stderr,"open %s error\n",file);
		return false;
	}
	

	char buffer[1024] = {0};
	//in.getline(buffer,1024);
	while(!in.eof())
	{
		memset(buffer,0,sizeof(buffer));
		if(in.getline(buffer,sizeof(buffer)) && '#' != buffer[0] && strlen(buffer) > 9)
		{
			vector<string> sigs = Strtok(buffer, " ");

			if(8 > sigs.size() || '#' == sigs[0][0])
			{
				continue;
			}
		
			string name = sigs[0];
			int id  = 0;
			sscanf(sigs[1].c_str(),"0x%x",&id);
			uint8_t start = (uint8_t)atoi(sigs[2].c_str());
			uint8_t len = (uint8_t)atoi(sigs[3].c_str());
			uint32_t interval = (uint32_t)atoi(sigs[4].c_str());
			int factor = atoi(sigs[5].c_str());
			int offset = atoi(sigs[6].c_str());
			string type = sigs[7];
			vector<DBCValue> dbcValue;
			vector<string> values = Strtok(sigs[8].c_str(), ",");
			for(auto v:values)
			{
				vector<string> a = Strtok(v.c_str(), ":");
				if(a.size() == 2)
				{
					DBCValue dc;
					dc.value = atoi(a[0].c_str());
					dc.name = a[1];
					dbcValue.push_back(dc);
				}
			}
			
			dbcList.emplace_back(name,id,start,len,interval,dbcValue);
		}
	}
}

void help()
{
	printf("can_tool <dbcfile> <canfile>\n");
	printf("like:\n");
	printf("  can_tool ../S16DBC.txt ../../testfile/can4.txt\n");
	printf("  can_tool ../S16DBC.txt ../../testfile/110-65535.csv\n");
}

int main(int argc,char** argv)
{
	if(argc < 3)
	{
		help();
		return 0;
	}
	
	ifstream in;
	in.open(argv[2], ios::binary);
	if (!in.is_open())
	{
		fprintf(stderr,"open %s failed",argv[2]);
		return 0;
	}
	char* tok = "\t";
	if(strstr(argv[2],".csv"))
	{
		tok = ",";
	}

	vector<MiniDBC> g_DBC;
	ParseMiniDBCFile(argv[1],g_DBC);
	
	char buffer[1024] = { 0 };
	const int dbcLen = g_DBC.size();
	vector<CanInfo_t> cans;
	uint64_t value = 0;
	
	int line = 1;
	in.getline(buffer,1024);
	line++;

	//printf("series,x,y\r\n");
	std::string outFile = string(argv[2]) + ".out";
	
	cout<<"outFile:"<<outFile<<endl;
	ofstream outFileOS(outFile,ios::binary);
	while (!in.eof())
	{
		memset(buffer, 0, sizeof(buffer));
		
		if (in.getline(buffer, 1024))
		{
			CanInfo_t can = ParseCan(buffer,line,tok);
			#if 1
			for(int i = 0;i < dbcLen; i++)
			{
				if(can.id == g_DBC[i].id)
				{
					DBCValue dbcValue;
					dbcValue.value = g_DBC[i].parse(can.data);
					uint64_t cur = ToMSecond(can.time);
					uint64_t lastTime = ToMSecond(g_DBC[i].lastTime);
					
					if(0 == lastTime || g_DBC[i].lastValue != dbcValue.value)
					{
						char buffer[128] = {0};

						for(auto v:g_DBC[i].valueMap)
						{
							if(v.value == dbcValue.value)
							{
								dbcValue.name = v.name;
								break;
							}
						}
						
						int len = sprintf(buffer,"%s  %s  %s[%lu]\r\n",StrCanTime(can.time).c_str(),g_DBC[i].name.c_str(),dbcValue.name.c_str(), dbcValue.value); 
						outFileOS.write(buffer, len);
						g_DBC[i].lastValue = dbcValue.value;
					}

					//判断相邻两帧间隔是否过大
					if(lastTime != 0 && cur - lastTime > g_DBC[i].interval)
					{
						fprintf(stderr,"%s interval:%lu ERROR, lastTime:%s line:%d\n",g_DBC[i].name.c_str(), cur - lastTime, StrCanTime(g_DBC[i].lastTime).c_str(),can.line);
					}
					g_DBC[i].lastTime = can.time;
					
					//printf("%s,%lu,%lu\r\n",g_DBC[i].name.c_str(), ToMSecond(can.time), value);
				}
				
			}
			#endif
			cans.push_back(can);
		}
		
		line++;
	}
	outFileOS.close();
	#if 0
	for(int i = 0;i < dbcLen; i++)
	{
		std::string csvFile = g_DBC[i].name + ".csv";
		cout<<"output:"<<csvFile<<endl;
		ofstream outbin(csvFile,ios::binary);
		char buffer[128] = {0};
		int len = sprintf(buffer,"time,value\r\n");
		outbin.write(buffer, len);
		for(auto& can: cans)
		{
			if(g_DBC[i].parse(&can, &value))
			{
				
				uint64_t cur = ToMSecond(can.time);
				uint64_t lastTime = ToMSecond( g_DBC[i].lastTime);
				uint64_t lastWriteTime = ToMSecond( g_DBC[i].lastWriteTime);
				//判断相邻两帧间隔是否过大
				if(lastTime != 0 && cur - lastTime > g_DBC[i].intveval)
				{
					fprintf(stderr,"%s interval:%lu ERROR, lastTime:%s line:%d\n",g_DBC[i].name.c_str(), cur - lastTime, StrCanTime(g_DBC[i].lastTime).c_str(),can.line);
				}
					
		
				if(g_DBC[i].lastValue != value || cur - lastWriteTime > 5000)
				{
					len = sprintf(buffer,"%s,%lu\r\n",StrCanTime(can.time).c_str(), value); 
					outbin.write(buffer, len);
					//printf("%s,%lu cur:%lu last:%lu\n",StrCanTime(can.time).c_str(), value,cur,g_DBC[i].lastTime);
					g_DBC[i].lastWriteTime = can.time;
				}
				g_DBC[i].lastValue = value;
				g_DBC[i].lastTime = can.time;
				//printf("%s,%lu\r\n",StrCanTime(can.time).c_str(), value);
				//len = sprintf(buffer,"%s,%lu\r\n",StrCanTime(can.time).c_str(), value);	
				//outbin.write(buffer, len);
			}
		}
	}
	#endif
	return 0;
}
