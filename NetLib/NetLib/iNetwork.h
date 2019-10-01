#pragma once 

// 패킷 처리 스레드 결정하기
enum ePROCESS_MODE : DWORD
{
	PROCESS_MODE_USER,		// 패킷 처리를 유저의 다른 스레드에서 땡겨가서 처리할께요
	PROCESS_MODE_OWN		// 패킷 처리를 내 proc 스레드로 이용 할께염
};

// 세션 대리
class iSessionProxy
{
public:
	virtual bool IsConnected() const = 0;
	// Send을 요청한다.
	virtual bool PostSend(void* Data, unsigned int Len) = 0;
	// 버퍼에 채움
	virtual bool PostSendBuffered(void* Data, unsigned int Len) = 0;
	// Send버퍼를 Flush 시킨다.
	virtual bool PostSend() = 0;
	// 패킷 처리
	virtual int ProcessPacket() = 0;
	// SessionID 구하기
	virtual int GetSessionID() const = 0;
	// 원격 클라이언트의 접속 ip 구하기
	virtual const wchar_t*	GetPeerIP() const = 0;
	// 원격 클라이언트의 접속 포트 구하기
	virtual const unsigned short GetPeerPort() const = 0;
	// 접속 할 서버의 IP (클라이언트인 경우)
	virtual const wchar_t* GetTargetIP() const = 0;
	// 접속 할 서버의 PORT (클라이언트인 경우)
	virtual const unsigned short GetTargetPort() const = 0;
};

// 세션 stub
class iSessionStub
{
public:
	// Accept 되었음
	virtual void OnAccept(iSessionProxy* Proxy, const wchar_t* TargetAddress, const int TargetPort) = 0;
	// Connect 되었음
	virtual void OnConnect(iSessionProxy* Proxy, const wchar_t* TargetAddress, const int TargetPort) = 0;
	// 종료 되었음
	virtual void OnClose(iSessionProxy* Proxy) = 0;
	// dispatch 되었음
	virtual void OnDispatch(iSessionProxy* Proxy, char * Data, unsigned int Len) = 0;
};

using CreateClientSessionFunc = std::function<iSessionStub* ()>;

class iNetwork
{
public:
	// 시작 시킴
	// - session pool 을 만들고
	// - io / proc thread 들을 시작시킴
	virtual void Start(CreateClientSessionFunc CreateFunction) = 0;
	// 중단 시킴
	virtual void Shutdown() = 0;
	// Start Acceptor
	virtual void StartAcceptor(const wchar_t* ListenAddr, int ListenPort, bool NoDelay) = 0;
	// Stop Acceptor
	virtual void StopAcceptor() = 0;

	// 서버에 접속한다.
	// - blocking 
	virtual iSessionStub * ConnectSession(const wchar_t* TargetAddress, const int TargetPort) = 0;
	// 클라이언트의 소켓 접속을 강제로 끊는다.
	virtual void DisconnectSession(int SessionID) = 0;

	//-------------------------------------------------------------------------
	// MISC
	//-------------------------------------------------------------------------
	virtual ePROCESS_MODE GetProcessMode() const = 0;
	// 최대 세션수
	virtual size_t	GetMaxSessionCount() const = 0;
	// 현재 세션수 
	virtual unsigned short	GetSessionCount() const = 0;

	virtual bool IsConnected(unsigned short Index) = 0;
};


iNetwork* CreateNetwork(const wchar_t* NetworkName = L"", size_t MaxSessionCount = 100, size_t WorkerThreadCount = 1, size_t ProcessThreadCount = 1, ePROCESS_MODE eParse = PROCESS_MODE_OWN);
