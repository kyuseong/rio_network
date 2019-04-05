#include "NativeNetLibPCH.h"
#include "Exception.h"
#include "StackWalker.h"
#include "../../../A5/shared/atlconv_patch.h"
#include "CallStack.h"

CInnerException::CInnerException(const _tstring &where, const _tstring &message) : m_where(where), m_message(message), m_File(L"")), m_Line(0), m_msg(L""))
{
#ifdef _DEBUG
	CallStack stack; 
	m_StackWalker = SymbolLookup::Instance().GetSymbolString(stack);

	Trace::Error(L"Stack:%s \r\n", m_StackWalker.c_str());

// 	USES_CONVERSION;
// 
// 	cStackWalker sw;
// 	sw.ShowCallStack();
// 
// 	m_StackWalker = A2W_UTF8(sw.GetStackString());
#endif // #ifdef _DEBUG
}
CInnerException::CInnerException ( const _tstring &where, LPCTSTR fmt, ... ) : m_where(where), m_Line(0), m_msg(L""))
{
#ifdef _DEBUG
	CallStack stack;
	m_StackWalker = SymbolLookup::Instance().GetSymbolString(stack);

	Trace::Error(L"Stack:%s \r\n", m_StackWalker.c_str());
// 	USES_CONVERSION;
// 
// 	cStackWalker sw;
// 	sw.ShowCallStack();
// 
// 	m_StackWalker = A2W_UTF8(sw.GetStackString());
#endif // #ifdef _DEBUG

	wchar_t sLog[2048];
	va_list args;
	va_start( args, fmt );
	vswprintf_s( sLog, std::size(sLog), fmt, args );

	wchar_t sMsg[2048];
	swprintf_s(sMsg, std::size(sMsg), L"%s(%d): %s", m_File.c_str(), m_Line, sLog);
	m_msg = sMsg;
	va_end( args );
}
CInnerException::CInnerException ( LPCTSTR file, unsigned int line, LPCTSTR function, const _tstring& message, LPCTSTR fmt, ...)
:
m_File(file),
m_Line(line),
m_where(function)
{	
#ifdef _DEBUG
	CallStack stack;
	m_StackWalker = SymbolLookup::Instance().GetSymbolString(stack);

	Trace::Error(L"Stack:%s \r\n", m_StackWalker.c_str());

// 	USES_CONVERSION;
// 
// 	cStackWalker sw;
// 	sw.ShowCallStack();
// 
// 	m_StackWalker = A2W_UTF8(sw.GetStackString());
#endif // #ifdef _DEBUG

	wchar_t sLog[2048];
	va_list args;
	va_start( args, fmt );
	vswprintf_s( sLog, std::size(sLog), fmt, args );

	wchar_t sMsg[2048];
	swprintf_s(sMsg, std::size(sMsg), L"%s(%d): %s (%s)", m_File.c_str(), m_Line, sLog, message.c_str());
	m_msg = sMsg;
	va_end( args );
}

_tstring CInnerException::GetWhere() const
{
	return m_where;
}

_tstring CInnerException::GetMessage() const
{
	if(!m_msg.empty() )
		return m_msg;
	else
		return m_message;
}

void CInnerException::MessageBox(HWND hWnd) const
{
	Trace::Error(L"%s\r\n"),GetMessage().c_str());
	::MessageBox(hWnd, GetMessage().c_str(), GetWhere().c_str(), MB_ICONSTOP);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief 파일에다 예외 발생을 기록한다.
///
/// 단독으로 쓰기 위한 함수가 아니라, main 함수 내부의 __try/__except 구문에 쓰기 위한 함수다. 
/// 아래와 같은 방식으로 사용한다. 
///
/// <pre>
/// int main(...)
/// {
///    __try
///    {
///        ...
///    }
///    __except (LogException(GetExceptionInformation())
///    {
///    }
///    return 0;   
/// }
/// </pre>
///
/// 이와 같은 함수가 필요한 이유는 EXCEPTION_STACK_OVERFLOW가 발생한 경우, MiniDump 만으로는 이를 
/// 알 수가 없기 때문이다. 스택 오버플로우 시에는 함수 호출이 제대로 안 되기 때문에 덤프도 
/// 기록되지 않고, 프로세스가 그냥 사라져 버린다. 최소한 스택 오버플로우가 발생했다는 사실 정도는 
/// 알 수 있도록 이런 함수를 메인 함수에다 두어야 한다. 
///
/// \param exPtrs 익셉션 포인터
/// \return DWORD EXCEPTION_CONTINUE_SEARCH
////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD LogException(PEXCEPTION_POINTERS exPtrs)
{
	LPTSTR msg = NULL;

	// 간단한 에러 코드라면 그냥 변환할 수 있다.
	switch (exPtrs->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:         msg = L"EXCEPTION_ACCESS_VIOLATION"); break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:    msg = L"EXCEPTION_DATATYPE_MISALIGNMENT"); break;
	case EXCEPTION_BREAKPOINT:               msg = L"EXCEPTION_BREAKPOINT"); break;
	case EXCEPTION_SINGLE_STEP:              msg = L"EXCEPTION_SINGLE_STEP"); break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    msg = L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED"); break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:     msg = L"EXCEPTION_FLT_DENORMAL_OPERAND"); break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       msg = L"EXCEPTION_FLT_DIVIDE_BY_ZERO"); break;
	case EXCEPTION_FLT_INEXACT_RESULT:       msg = L"EXCEPTION_FLT_INEXACT_RESULT"); break;
	case EXCEPTION_FLT_INVALID_OPERATION:    msg = L"EXCEPTION_FLT_INVALID_OPERATION"); break;
	case EXCEPTION_FLT_OVERFLOW:             msg = L"EXCEPTION_FLT_OVERFLOW"); break;
	case EXCEPTION_FLT_STACK_CHECK:          msg = L"EXCEPTION_FLT_STACK_CHECK"); break;
	case EXCEPTION_FLT_UNDERFLOW:            msg = L"EXCEPTION_FLT_UNDERFLOW"); break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       msg = L"EXCEPTION_INT_DIVIDE_BY_ZERO"); break;
	case EXCEPTION_INT_OVERFLOW:             msg = L"EXCEPTION_INT_OVERFLOW"); break;
	case EXCEPTION_PRIV_INSTRUCTION:         msg = L"EXCEPTION_PRIV_INSTRUCTION"); break;
	case EXCEPTION_IN_PAGE_ERROR:            msg = L"EXCEPTION_IN_PAGE_ERROR"); break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:      msg = L"EXCEPTION_ILLEGAL_INSTRUCTION"); break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: msg = L"EXCEPTION_NONCONTINUABLE_EXCEPTION"); break;
	case EXCEPTION_STACK_OVERFLOW:           msg = L"EXCEPTION_STACK_OVERFLOW"); break;
	case EXCEPTION_INVALID_DISPOSITION:      msg = L"EXCEPTION_INVALID_DISPOSITION"); break;
	case EXCEPTION_GUARD_PAGE:               msg = L"EXCEPTION_GUARD_PAGE"); break;
	case EXCEPTION_INVALID_HANDLE:           msg = L"EXCEPTION_INVALID_HANDLE"); break;
	case CONTROL_C_EXIT:                     msg = L"CONTROL_C_EXIT"); break;
	case 0xE06D7363:                         msg = L"Microsoft C++ Exception"); break;
	case 0xE0434F4D:                         msg = L"Microsoft .NET Exception"); break;
	default:                                 msg = L"UNKNOWN"); break;
	}

	LogToFile(L"exception triggered: %s"), msg);

	if (exPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
		_resetstkoflw();

	return EXCEPTION_CONTINUE_SEARCH;
}
