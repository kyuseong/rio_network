#pragma once

#include "SocketConfig.h"

extern RIO_EXTENSION_FUNCTION_TABLE RIO;
extern std::atomic<bool> SupportRIO ;

namespace SocketUtils
{
	bool Startup();
	void Cleanup();
	DWORD GetAllocationGranularity();
	DWORD GetBestIOBufferSize(DWORD Buffer);
	DWORD GetNumberOfProcessors();
}
