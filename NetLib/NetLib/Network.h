#pragma once

#include "iNetwork.h"
#include "SocketConfig.h"
#include "Session.h"

class IOCompletionPort;
class NetworkIOModel;

// 네트워크
class Network : public iNetwork
{
private:
	std::wstring		m_NetworkName;			// 이름
	ePROCESS_MODE		m_ProcessMode;			// 패킷 처리를 어떻게 할것인지 처리 모드
	std::unique_ptr<NetworkIOModel> m_IOModel;
	bool m_Running = true;
	std::vector<std::unique_ptr<std::thread>> m_IOThreads;
	std::vector<std::unique_ptr<std::thread>> m_ProcessThreads;
	std::unique_ptr<std::thread> m_ClosingThread;
	std::unique_ptr<IOCompletionPort> m_ProcessIOCP;

	size_t				m_IoThreadCount;		// IO 스레드 수
	size_t				m_ProcessThreadCount;	// Process 스레드수
	size_t				m_MaxSessionCount;		// 최대 세션 수

	int*				m_SessionCountPerThread;// 스레드별로 소유하고 있는 세션 숫자
	//----------------------------------------------------------------------------
	// 세션
	//----------------------------------------------------------------------------
	SpinLock			m_SessionListLock;		// 리스트 lock
	Session**			m_ActiveSessionList;	// 현재 세션 (ARRAY - 배열)
	std::deque<Session*> m_FreeSessionList;		// free 세션 리스트
	unsigned short		m_SessionCount;			// 현재 세션수
	
	//----------------------------------------------------------------------------
	// 종료 처리
	//----------------------------------------------------------------------------
	std::mutex m_ClosingSessionListLock;		// 리스트 lock
	std::deque<Session*> m_ClosingSessionList;	// 종료 세션 리스트
	std::condition_variable m_ClosingSessionCondition;
	
	//----------------------------------------------------------------------------
	// Acceptor
	//----------------------------------------------------------------------------
	std::wstring 		m_ListenAddress;		// listen address
	int					m_ListenPort;			// listen port
	SOCKET				m_ListenSocket;			// listen 소켓
	std::unique_ptr<std::thread> m_Acceptor;	// acceptor thread
	bool m_RunningAcceptor = false;
public:
	explicit Network(
			const wchar_t* NetworkName, 
			size_t MaxSessionCount, 
			size_t WorkerThreadCount, 
			size_t ProcessThreadCount, ePROCESS_MODE eParse = PROCESS_MODE_OWN);
	~Network();

	//-------------------------------------------------------------------------
	// 서버 구동
	//-------------------------------------------------------------------------
	
	// 시작 시킴
	// - session pool 을 만들고
	// - io / proc thread 들을 시작시킴
	virtual void Start(CreateClientSessionFunc CreateFunction) override;
	// 중단 시킴
	void Shutdown() override;
	// Start Acceptor
	virtual void StartAcceptor(const wchar_t* ListenAddr, int ListenPort, bool NoDelay) override;
	// Stop Acceptor
	virtual void StopAcceptor() override;

	// 서버에 접속한다.
	// - blocking 
	iSessionStub * ConnectSession(const wchar_t* TargetAddress, const int TargetPort) override;
	// 클라이언트의 소켓 접속을 강제로 끊는다.
	void DisconnectSession(Session* Sess);
	// 
	virtual void DisconnectSession(int SessionID);	
	//-------------------------------------------------------------------------
	// MISC
	//-------------------------------------------------------------------------
	virtual ePROCESS_MODE	GetProcessMode() const override { return m_ProcessMode; }

	//소켓 id를 구한다.
	Session* GetSession(unsigned short SessionID) { return SessionID < m_MaxSessionCount ? m_ActiveSessionList[SessionID] : NULL; }
	// 최대 세션수
	virtual size_t	GetMaxSessionCount() const override { return m_MaxSessionCount; }
	// 현재 세션수 
	virtual unsigned short	GetSessionCount() const override { return m_SessionCount; }

	virtual bool IsConnected(unsigned short nIndex) override;

	//-------------------------------------------------------------------------
	// 스레드
	//-------------------------------------------------------------------------
private:
	// io thread 와 process thread 을 시작한다.
	void StartThread();

	// io 처리 스레드
	int IoWorkerThreadEntry(unsigned int ThreadId);
	// 패칫 처리 스레드
	int ProcessWorkerThreadEntry();
	// 크로징 시키는 스레드
	int ClosingThreadEntry();
	// 클라이언트의 세션을 해제한다.
	void AddClosingSession(Session * Session, const wchar_t * SrcFile, const unsigned int SrcLine);

	//-------------------------------------------------------------------------
	// 세션 관리
	//-------------------------------------------------------------------------

	// 세션 할당
	Session* AllocateSession(SOCKET hSocket);
	// 세션 해제
	void ReleaseSession(Session * Session, const wchar_t * SrcFile, const unsigned int SrcLine);
	// 세션 풀 만들기
	void CreateSessionPool(CreateClientSessionFunc CreateFunction);
	// 세션 풀 해제하기
	void DestroySessionPool();
	// 세션 들이 모두 종료할때 까지 기다림
	bool WaitForClosedSession();
	// 세션 풀에 세션들을 넣음
	void PushFreeSession(Session * Session);

private:
	//-------------------------------------------------------------------------
	// 큐
	//-------------------------------------------------------------------------
	void*	GetCompletionQueue(unsigned int ThreadId) const;

	void	EnqueueProcess(ULONG_PTR completionKey, DWORD dwNumBytes, OVERLAPPED *pOverlapped) const;
	bool	DequeueProcess(Session *&Session, DWORD &dwIOSize, WSAOVERLAPPED *&pOvl, DWORD &dwLastError) const;

	SOCKET CreateListenSocket(const wchar_t* Address, unsigned short port, bool NoDelay);
	int AcceptThread();

	// 초기화
	// 적절한 ThreadID를 구하고 카운트를 늘려준다.
	int GetProperThreadIDAndIncCount();
	void DecSessionCountPerThread(Session * Session);

	friend class Session;
	friend class NetworkIOModel_IOCP;
	friend class NetworkIOModel_RIO;
	friend class SessionIOModel_RIO;
};
