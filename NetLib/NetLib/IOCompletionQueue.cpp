#include "NetLibPCH.h"
#include "IOCompletionQueue.h"
#include "Output.h" 

IOCompletionPort::IOCompletionPort(size_t maxConcurrency)
{
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, static_cast<DWORD>(maxConcurrency));
}

IOCompletionPort::~IOCompletionPort()
{
	::CloseHandle(m_hIOCP);
}

void IOCompletionPort::AssociateDevice(HANDLE hHandle, ULONG_PTR CompletionKey) const
{
	::CreateIoCompletionPort(hHandle, m_hIOCP, CompletionKey, 0);
}

bool IOCompletionPort::PostStatus(ULONG_PTR CompletionKey, DWORD dwNumbytes, OVERLAPPED *pOverlapped) const
{
	return ::PostQueuedCompletionStatus(m_hIOCP, dwNumbytes, CompletionKey, pOverlapped);
}

bool IOCompletionPort::GetCompletionStatus(ULONG_PTR *pCompletionKey, PDWORD pdwNumbytes, OVERLAPPED **ppOverlapped) const
{
	return ::GetQueuedCompletionStatus(m_hIOCP, pdwNumbytes, pCompletionKey, ppOverlapped, INFINITE);
}

bool IOCompletionPort::GetCompletionStatus(ULONG_PTR *pCompletionKey, PDWORD pdwNumbytes, OVERLAPPED **ppOverlapped, DWORD dwMilliseconds) const
{
	bool bRet = true;
	if ( 0 == ::GetQueuedCompletionStatus(m_hIOCP, pdwNumbytes, pCompletionKey, ppOverlapped, dwMilliseconds)) {
		DWORD dwLastError = ::GetLastError();
		if (dwLastError != WAIT_TIMEOUT) {
			return false;
		}
		bRet = false;
	}
	return bRet;
}

bool IOCompletionPort::GetCompletionStatus(ULONG_PTR *pCompletionKey, PDWORD pdwNumbytes, OVERLAPPED **ppOverlapped, DWORD *pLastError) const
{
	bool bRet = true;
	if (0 == ::GetQueuedCompletionStatus(m_hIOCP, pdwNumbytes, pCompletionKey, ppOverlapped, INFINITE)) {
		if (pLastError)	
			*pLastError = ::GetLastError();
		bRet = false;
	}
	return bRet;
}

