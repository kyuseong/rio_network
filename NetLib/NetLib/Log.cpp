#include "NetLibPCH.h"
#include "Log.h" 

#include <stdio.h>

void Output(const wchar_t * file, const unsigned int line, const wchar_t * pszFormat, ...)
{
	wchar_t sLog[ 2048 ];
	wchar_t sOutputLog[2048] = {0,};

	va_list args;
	va_start( args, pszFormat );
	vswprintf( sLog, std::size(sLog), pszFormat, args );
	va_end( args );

	swprintf(sOutputLog, std::size(sOutputLog), L"%s(%u):[%d]%s \r\n", file, line, GetTickCount(), sLog);

	OutputDebugString(sOutputLog);
}

void Output(const wchar_t * pszFormat, ...)
{
	UNREFERENCED_PARAMETER(pszFormat);

#ifdef _DEBUG
	wchar_t sLog[2048];

	va_list args;
	va_start( args, pszFormat );

	vswprintf( sLog, std::size(sLog), pszFormat, args );
	va_end( args );

	OutputDebugString(sLog);
#endif
}

std::wstring GetLastErrorMessage(DWORD dwError)
{
	static wchar_t errmsg[2048];

	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, dwError, 0, errmsg, 511, nullptr)) {
		return GetLastErrorMessage(GetLastError());
	}

	return errmsg;
}