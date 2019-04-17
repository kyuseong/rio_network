// DummyClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "DummyClient.h"
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		wprintf(L"usage: server.exe 0.0.0.0 12000\n");

		return -1;
	}

	std::wstring Address = s2ws(argv[1]);
	int Port = atoi(argv[2]);

	setlocale(LC_ALL, "Korean");

	while (1)
	{

		g_DummyClient->Connect(Address.c_str(), Port);

		Sleep(1000);

		g_DummyClient->Close();

		Sleep(1000);
	}
	g_DummyClient->Shutdown();

	return 0;
}