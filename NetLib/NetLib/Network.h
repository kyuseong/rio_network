#pragma once

#include "iNetwork.h"
#include "SocketConfig.h"
#include "Session.h"

class IOCompletionPort;
class NetworkIOModel;

// ��Ʈ��ũ
class Network : public iNetwork
{
private:
	std::wstring		m_NetworkName;			// �̸�
	ePROCESS_MODE		m_ProcessMode;			// ��Ŷ ó���� ��� �Ұ����� ó�� ���
	std::unique_ptr<NetworkIOModel> m_IOModel;
	bool m_Running = true;
	std::vector<std::unique_ptr<std::thread>> m_IOThreads;
	std::vector<std::unique_ptr<std::thread>> m_ProcessThreads;
	std::unique_ptr<std::thread> m_ClosingThread;
	std::unique_ptr<IOCompletionPort> m_ProcessIOCP;

	size_t				m_IoThreadCount;		// IO ������ ��
	size_t				m_ProcessThreadCount;	// Process �������
	size_t				m_MaxSessionCount;		// �ִ� ���� ��

	int*				m_SessionCountPerThread;// �����庰�� �����ϰ� �ִ� ���� ����
	//----------------------------------------------------------------------------
	// ����
	//----------------------------------------------------------------------------
	SpinLock			m_SessionListLock;		// ����Ʈ lock
	Session**			m_ActiveSessionList;	// ���� ���� (ARRAY - �迭)
	std::deque<Session*> m_FreeSessionList;		// free ���� ����Ʈ
	unsigned short		m_SessionCount;			// ���� ���Ǽ�
	
	//----------------------------------------------------------------------------
	// ���� ó��
	//----------------------------------------------------------------------------
	std::mutex m_ClosingSessionListLock;		// ����Ʈ lock
	std::deque<Session*> m_ClosingSessionList;	// ���� ���� ����Ʈ
	std::condition_variable m_ClosingSessionCondition;
	
	//----------------------------------------------------------------------------
	// Acceptor
	//----------------------------------------------------------------------------
	std::wstring 		m_ListenAddress;		// listen address
	int					m_ListenPort;			// listen port
	SOCKET				m_ListenSocket;			// listen ����
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
	// ���� ����
	//-------------------------------------------------------------------------
	
	// ���� ��Ŵ
	// - session pool �� �����
	// - io / proc thread ���� ���۽�Ŵ
	virtual void Start(CreateClientSessionFunc CreateFunction) override;
	// �ߴ� ��Ŵ
	void Shutdown() override;
	// Start Acceptor
	virtual void StartAcceptor(const wchar_t* ListenAddr, int ListenPort, bool NoDelay) override;
	// Stop Acceptor
	virtual void StopAcceptor() override;

	// ������ �����Ѵ�.
	// - blocking 
	iSessionStub * ConnectSession(const wchar_t* TargetAddress, const int TargetPort) override;
	// Ŭ���̾�Ʈ�� ���� ������ ������ ���´�.
	void DisconnectSession(Session* Sess);
	// 
	virtual void DisconnectSession(int SessionID);	
	//-------------------------------------------------------------------------
	// MISC
	//-------------------------------------------------------------------------
	virtual ePROCESS_MODE	GetProcessMode() const override { return m_ProcessMode; }

	//���� id�� ���Ѵ�.
	Session* GetSession(unsigned short SessionID) { return SessionID < m_MaxSessionCount ? m_ActiveSessionList[SessionID] : NULL; }
	// �ִ� ���Ǽ�
	virtual size_t	GetMaxSessionCount() const override { return m_MaxSessionCount; }
	// ���� ���Ǽ� 
	virtual unsigned short	GetSessionCount() const override { return m_SessionCount; }

	virtual bool IsConnected(unsigned short nIndex) override;

	//-------------------------------------------------------------------------
	// ������
	//-------------------------------------------------------------------------
private:
	// io thread �� process thread �� �����Ѵ�.
	void StartThread();

	// io ó�� ������
	int IoWorkerThreadEntry(unsigned int ThreadId);
	// ��ĩ ó�� ������
	int ProcessWorkerThreadEntry();
	// ũ��¡ ��Ű�� ������
	int ClosingThreadEntry();
	// Ŭ���̾�Ʈ�� ������ �����Ѵ�.
	void AddClosingSession(Session * Session, const wchar_t * SrcFile, const unsigned int SrcLine);

	//-------------------------------------------------------------------------
	// ���� ����
	//-------------------------------------------------------------------------

	// ���� �Ҵ�
	Session* AllocateSession(SOCKET hSocket);
	// ���� ����
	void ReleaseSession(Session * Session, const wchar_t * SrcFile, const unsigned int SrcLine);
	// ���� Ǯ �����
	void CreateSessionPool(CreateClientSessionFunc CreateFunction);
	// ���� Ǯ �����ϱ�
	void DestroySessionPool();
	// ���� ���� ��� �����Ҷ� ���� ��ٸ�
	bool WaitForClosedSession();
	// ���� Ǯ�� ���ǵ��� ����
	void PushFreeSession(Session * Session);

private:
	//-------------------------------------------------------------------------
	// ť
	//-------------------------------------------------------------------------
	void*	GetCompletionQueue(unsigned int ThreadId) const;

	void	EnqueueProcess(ULONG_PTR completionKey, DWORD dwNumBytes, OVERLAPPED *pOverlapped) const;
	bool	DequeueProcess(Session *&Session, DWORD &dwIOSize, WSAOVERLAPPED *&pOvl, DWORD &dwLastError) const;

	SOCKET CreateListenSocket(const wchar_t* Address, unsigned short port, bool NoDelay);
	int AcceptThread();

	// �ʱ�ȭ
	// ������ ThreadID�� ���ϰ� ī��Ʈ�� �÷��ش�.
	int GetProperThreadIDAndIncCount();
	void DecSessionCountPerThread(Session * Session);

	friend class Session;
	friend class NetworkIOModel_IOCP;
	friend class NetworkIOModel_RIO;
	friend class SessionIOModel_RIO;
};
