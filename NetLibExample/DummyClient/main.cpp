// DummyClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "DummyClient.h"
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>
#include <iostream>

using namespace std;

int main(int , TCHAR** )
{
	setlocale(LC_ALL, "Korean");

	while (1)
	{
		g_DummyClient->ConnectServer();

	Sleep(1000);

		g_DummyClient->CloseChatServer();

	}
	g_DummyClient->Shutdown();

	return 0;
}