#pragma once

void Output(const wchar_t * file, const unsigned int line, const wchar_t * pszFormat, ...);
void Output(const wchar_t * pszFormat, ...);

#ifdef _DEBUG
	#define ERRORLOG(...)		Output(__WFILE__, __LINE__, __VA_ARGS__)
#else
	#define ERRORLOG(...)		__noop
#endif

#ifdef _UNICODE
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WIDEN2__(x) L ## x
#define __WIDEN__(x) __WIDEN2__(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WFUNCTION__ __WIDEN__(__FUNCTION__)
#endif

std::wstring GetLastErrorMessage(DWORD dwError);

inline const std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length();
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	std::wstring r(len, L'\0');
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
	return r;
}


inline const std::string ws2s(const std::wstring& s)
{
	int len;
	int slength = (int)s.length();
	len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	std::string r(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}
