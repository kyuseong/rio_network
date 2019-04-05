#include "NetLibPCH.h"
#include "SocketUtils.h"

RIO_EXTENSION_FUNCTION_TABLE RIO{};
std::atomic<bool> SupportRIO = false;

namespace SocketUtils
{
	bool Startup()
	{
		// 윈속 초기화
		WSADATA Wsa{};
		if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0)
			return false;

		// RIO function table 얻어오기
		GUID FunctionTableId = WSAID_MULTIPLE_RIO;
		DWORD Bytes = 0;
		SOCKET Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO);
		if (WSAIoctl(Socket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &FunctionTableId, sizeof(GUID),
			(void**)&RIO, sizeof(RIO), &Bytes, NULL, NULL))
		{
			closesocket(Socket);
			SupportRIO = false;
			return false;
		}
		
		SupportRIO = true;

		closesocket(Socket);
		return true;
	}

	void Cleanup()
	{
		WSACleanup();
	}

	DWORD GetAllocationGranularity()
	{
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		return systemInfo.dwAllocationGranularity; ///< maybe 64K
	}

	DWORD GetBestIOBufferSize(DWORD Buffer)
	{
		const DWORD AllocationGranularity = GetAllocationGranularity();
		return (Buffer / AllocationGranularity + 1) * AllocationGranularity;
	}

	DWORD GetNumberOfProcessors()
	{
		SYSTEM_INFO		SysInfo;
		GetSystemInfo(&SysInfo);
		return SysInfo.dwNumberOfProcessors;
	}
}