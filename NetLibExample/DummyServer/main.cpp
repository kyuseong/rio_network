// ChatServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "../Packet/Packet.h"
#include "DummySession.h"
#include "DummyServer.h"

extern std::atomic<int> g_Count;

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		wprintf(L"usage: server.exe 0.0.0.0 12000\n");

		return -1;
	}
	
	std::wstring Address = s2ws(argv[1]);
	int Port = atoi( argv[2] );

	wprintf(L"start\n");

	setlocale(LC_ALL, "Korean");

	DummyServer::GetInstance()->Start(Address.c_str(), Port);

	int Seq = 0;
	while (1)
	{
		ans_chat_msg1 SendPacket;
		SendPacket.m_No = Seq++;

		DummyServer::GetInstance()->Broadcast(reinterpret_cast<char *>(&SendPacket), SendPacket.Size);

		//DummyServer::GetInstance()->Check();

		Sleep(1);

		g_Count = 0;
	}

	DummyServer::GetInstance()->Shutdown();

}
