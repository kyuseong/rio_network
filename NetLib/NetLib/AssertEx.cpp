#include "NetLibPCH.h"
#include "AssertEx.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief 콜스택을 파일에다 기록하는 어서트 함수
/// \param fileName 어서트가 발생한 파일 이름
/// \param line 어서트가 발생한 라인
/// \param func 어서트가 발생한 함수
/// \param expr 어서트가 발생한 표현식(expression)
////////////////////////////////////////////////////////////////////////////////////////////////////
void FireAssertionFailureA(LPCSTR fileName, DWORD line, LPCSTR func, LPCSTR expr)
{
	DebugBreak();
}

/////////////////////////////////////////////////////l//////////////////////////////////////////////
/// \brief 콜스택을 파일에다 기록하는 어서트 함수
/// \param fileName 어서트가 발생한 파일 이름
/// \param line 어서트가 발생한 라인
/// \param func 어서트가 발생한 함수
/// \param expr 어서트가 발생한 표현식(expression)
////////////////////////////////////////////////////////////////////////////////////////////////////
void FireAssertionFailureW(LPCWSTR fileName, DWORD line, LPCWSTR func, LPCWSTR expr)
{
	DebugBreak();
}
