#pragma once 

// ��Ŷ ó�� ������ �����ϱ�
enum ePROCESS_MODE : DWORD
{
	PROCESS_MODE_USER,		// ��Ŷ ó���� ������ �ٸ� �����忡�� ���ܰ��� ó���Ҳ���
	PROCESS_MODE_OWN		// ��Ŷ ó���� �� proc ������� �̿� �Ҳ���
};

// ���� �븮
class iSessionProxy
{
public:
	virtual bool IsConnected() const = 0;
	// Send�� ��û�Ѵ�.
	virtual bool PostSend(void* Data, unsigned int Len) = 0;
	// ���ۿ� ä��
	virtual bool PostSendBuffered(void* Data, unsigned int Len) = 0;
	// Send���۸� Flush ��Ų��.
	virtual bool PostSend() = 0;
	// ��Ŷ ó��
	virtual int ProcessPacket() = 0;
	// SessionID ���ϱ�
	virtual int GetSessionID() const = 0;
	// ���� Ŭ���̾�Ʈ�� ���� ip ���ϱ�
	virtual const wchar_t*	GetPeerIP() const = 0;
	// ���� Ŭ���̾�Ʈ�� ���� ��Ʈ ���ϱ�
	virtual const unsigned short GetPeerPort() const = 0;
	// ���� �� ������ IP (Ŭ���̾�Ʈ�� ���)
	virtual const wchar_t* GetTargetIP() const = 0;
	// ���� �� ������ PORT (Ŭ���̾�Ʈ�� ���)
	virtual const unsigned short GetTargetPort() const = 0;
};

// ���� stub
class iSessionStub
{
public:
	// Accept �Ǿ���
	virtual void OnAccept(iSessionProxy* Proxy, const wchar_t* TargetAddress, const int TargetPort) = 0;
	// Connect �Ǿ���
	virtual void OnConnect(iSessionProxy* Proxy, const wchar_t* TargetAddress, const int TargetPort) = 0;
	// ���� �Ǿ���
	virtual void OnClose(iSessionProxy* Proxy) = 0;
	// dispatch �Ǿ���
	virtual void OnDispatch(iSessionProxy* Proxy, char * Data, unsigned int Len) = 0;
};

using CreateClientSessionFunc = std::function<iSessionStub* ()>;

class iNetwork
{
public:
	// ���� ��Ŵ
	// - session pool �� �����
	// - io / proc thread ���� ���۽�Ŵ
	virtual void Start(CreateClientSessionFunc CreateFunction) = 0;
	// �ߴ� ��Ŵ
	virtual void Shutdown() = 0;
	// Start Acceptor
	virtual void StartAcceptor(const wchar_t* ListenAddr, int ListenPort, bool NoDelay) = 0;
	// Stop Acceptor
	virtual void StopAcceptor() = 0;

	// ������ �����Ѵ�.
	// - blocking 
	virtual iSessionStub * ConnectSession(const wchar_t* TargetAddress, const int TargetPort) = 0;
	// Ŭ���̾�Ʈ�� ���� ������ ������ ���´�.
	virtual void DisconnectSession(int SessionID) = 0;

	//-------------------------------------------------------------------------
	// MISC
	//-------------------------------------------------------------------------
	virtual ePROCESS_MODE GetProcessMode() const = 0;
	// �ִ� ���Ǽ�
	virtual size_t	GetMaxSessionCount() const = 0;
	// ���� ���Ǽ� 
	virtual unsigned short	GetSessionCount() const = 0;

	virtual bool IsConnected(unsigned short Index) = 0;
};


iNetwork* CreateNetwork(const wchar_t* NetworkName = L"", size_t MaxSessionCount = 100, size_t WorkerThreadCount = 1, size_t ProcessThreadCount = 1, ePROCESS_MODE eParse = PROCESS_MODE_OWN);
