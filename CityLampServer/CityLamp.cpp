#include "StdAfx.h"
#include "CityLamp.h"

Mutex CityLamp::LampID2LampMutexSocketMapMutex;
CityLamp::CityLamp(void)
{
}

int CityLamp::SendDataToLampbyID(u_short lampid, char* buf, int data_len)
{
	int data_sent = 0;
	//查找map中是否存在与lampid对应的sock和mutex
	map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.find(lampid);
	if (map_it != LampID2LampMutexSockMap.end())	//根据找到的Mutex对sock进行互斥写入
	{
		SOCKET sock = LampID2LampMutexSockMap[lampid]->sock;
		Mutex *pmutex = LampID2LampMutexSockMap[lampid]->lpMutex;
		{
			MutexGuard guard(*pmutex);
			data_sent = Sendn(sock, buf, data_len);
		}
		if (data_sent < data_len)
		{
			delete map_it->second;
			{
				MutexGuard guard(LampID2LampMutexSocketMapMutex);
				LampID2LampMutexSockMap.erase(lampid);						
			}
		}
	}
	else		//不存在与lampid对应的sock和mutex
	{
		return -1;
	}
	return data_sent;
}

void CityLamp::RemoveLampID(u_short lampid)
{
	{
		MutexGuard guard(LampID2LampMutexSocketMapMutex);
		map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.find(lampid);
		if (map_it != LampID2LampMutexSockMap.end())	//根据找到的Mutex对sock进行互斥写入
		{
			delete map_it->second;
			LampID2LampMutexSockMap.erase(lampid);
		}
	}
}

bool CityLamp::QueryLampID(u_short lampid)
{
	{
		MutexGuard guard(LampID2LampMutexSocketMapMutex);
		map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.find(lampid);
		if (map_it != LampID2LampMutexSockMap.end())	//根据找到的Mutex对sock进行互斥写入
			return TRUE;
		else return FALSE;
	}
}

void CityLamp::SetLampID2LampMutexSockMap(u_short id, LPLampMutexSockStruct lpLMSS)
{
	{
		MutexGuard guard(LampID2LampMutexSocketMapMutex);
		LampID2LampMutexSockMap[id] = lpLMSS;
	}
}

void CityLamp::SendOfflineCmdToPCClient(u_short lampid)
{
	char buf[7];
	buf[0] = 0x5A;
	buf[1] = 0x06;
	char* pid = (char*)&lampid;
	buf[2] = pid[0];
	buf[3] = pid[1];
	buf[4] = 0xFF;
	buf[5] = 0x00;
	buf[6] = 0x00;
	AddCheckSum(buf, sizeof(buf));
	SendDataToPCClient(buf, sizeof(buf));
}

void CityLamp::SendOnlineCmdToPCClient()
{
	char buf[7];
	MutexGuard guard(LampID2LampMutexSocketMapMutex);
	map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.begin();
	while (map_it != LampID2LampMutexSockMap.end())
	{
		u_short lampid = map_it->first;
		buf[0] = 0x5A;
		buf[1] = 0x06;
		char* pid = (char*)&lampid;
		buf[2] = pid[0];
		buf[3] = pid[1];
		buf[4] = 0xFE;
		buf[5] = 0x00;
		buf[6] = 0x00;
		AddCheckSum(buf, sizeof(buf));
		SendDataToPCClient(buf, sizeof(buf));
		++map_it;
	}
}
void CityLamp::ClearLampID2LampMutexSockMap(void)
{
	MutexGuard guard(LampID2LampMutexSocketMapMutex);
	map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.begin();
	while (map_it != LampID2LampMutexSockMap.end())
	{
		delete map_it->second;
		map_it = LampID2LampMutexSockMap.erase(map_it);
	}
	
	//确保全部删除
	LampID2LampMutexSockMap.clear();
	/*
	while (map_it != LampID2LampMutexSockMap.end())
	{
	//SendOfflineCmdToPCClient(map_it->first);
	//closemysocket(map_it->second.sock);
	shutdown(map_it->second.sock, SD_BOTH);
	//UpdateLampListView(false, map_it->first);
	++map_it;
	}
	*/
	//LampID2LampMutexSockMap.clear();
}

CityLamp::~CityLamp(void)
{
	//ClearLampID2LampMutexSockMap();
}
