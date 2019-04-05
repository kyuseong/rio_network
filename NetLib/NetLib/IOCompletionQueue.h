#pragma once

#include "SocketConfig.h"
#include "SocketUtils.h"

class IOCompleteQueue
{
public:
	virtual void* GetHandle(unsigned int ThreadID) const = 0;
};

class IOCompletionPort : IOCompleteQueue
{
	HANDLE m_hIOCP;
public:
	explicit IOCompletionPort(size_t maxConcurrency);
	~IOCompletionPort();

	void AssociateDevice(HANDLE hHandle, ULONG_PTR CompletionKey) const;
	bool PostStatus(ULONG_PTR CompletionKey, DWORD dwNumbytes, OVERLAPPED *pOverlapped = 0) const;
	bool GetCompletionStatus(ULONG_PTR *pCompletionKey, PDWORD pdwNumbytes, OVERLAPPED **ppOverlapped) const;
	bool GetCompletionStatus(ULONG_PTR *pCompletionKey, PDWORD pdwNumbytes, OVERLAPPED **ppOverlapped, DWORD dwMilliseconds) const;
	bool GetCompletionStatus(ULONG_PTR *pCompletionKey, PDWORD pdwNumbytes, OVERLAPPED **ppOverlapped, DWORD *pLastError) const;

	virtual void* GetHandle(unsigned int ThreadID) const {
		UNREFERENCED_PARAMETER(ThreadID);
		return m_hIOCP;
	}

};

class RIOBuffer;
class RIOCompletionQueue final : IOCompleteQueue 
{
	size_t THREAD_ID_COUNT;
	size_t MAX_CQ_SIZE;
	RIO_CQ* m_CQ = nullptr;
public:
	RIOCompletionQueue(size_t ThreadIdCount, size_t MaxCQSize)
		: THREAD_ID_COUNT(ThreadIdCount)
		, MAX_CQ_SIZE(MaxCQSize)
	{
		m_CQ = new RIO_CQ[THREAD_ID_COUNT]{};
		for (unsigned int i = 0;i < THREAD_ID_COUNT;++i)
		{
			m_CQ[i] = RIO.RIOCreateCompletionQueue((DWORD)MAX_CQ_SIZE, 0);
			if (m_CQ[i] == RIO_INVALID_CQ)
			{
				Assert(false);
				return;
			}
		}
	}
	~RIOCompletionQueue()
	{
		for (unsigned int i = 0;i < THREAD_ID_COUNT;++i)
		{
			RIO.RIOCloseCompletionQueue(m_CQ[i]);
		}
	}

	virtual void* GetHandle(unsigned int ThreadId) const override
	{
		Assert(ThreadId < THREAD_ID_COUNT);

		return m_CQ[ThreadId];
	}
};

class RIOResultSet
{
	RIORESULT* m_Buff;
	int m_Size;
public:
	RIOResultSet()
	{
		m_Size = 256;
		m_Buff = new RIORESULT[m_Size];
	}

	void Update(int ResultNums)
	{
		if (ResultNums >= m_Size)
		{
			// 오모 꽉차서 오네
			if (m_Size + 256 < 2048)
			{
				delete m_Buff;
				m_Size += 256;
				m_Buff = new RIORESULT[m_Size];
			}
		}
	}

	RIORESULT* GetData() const { return m_Buff; }
	int GetSize() const { return m_Size; }

	RIOBuffer* GetRIOBuffer(int Index) const {
		return reinterpret_cast<RIOBuffer*>(m_Buff[Index].RequestContext);
	}
	unsigned int GetBytesTransferred(int Index) const {
		return m_Buff[Index].BytesTransferred;
	}
};

