#pragma once 

#include "TypesCommon.h"

#include "Allocator.h"
#include "Network.h"
#include "Mutex.h"
#include "AssertEx.h"
#include "Output.h"


// 패킷 처리 스레드 결정하기
enum ePROCESS_MODE : DWORD
{
	PROCESS_MODE_USER,		// 패킷 처리를 유저의 다른 스레드에서 땡겨가서 처리할께요
	PROCESS_MODE_OWN		// 패킷 처리를 내 proc 스레드로 이용 할께염
};

using CreateClientSessionFunc = std::function<Session* (Network&, unsigned short)>;

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
	virtual Session * ConnectSession(const wchar_t* TargetAddress, const int TargetPort) = 0;
	// 클라이언트의 소켓 접속을 강제로 끊는다.
	virtual void DisconnectSession(Session * Session) = 0;

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

class iNetworkDelegate
{
public:
	virtual void OnAccept(const wchar_t* TargetAddress, const int TargetPort) = 0;
	virtual void OnConnect(const wchar_t* TargetAddress, const int TargetPort) = 0;
	virtual void OnClose() = 0;
	virtual void OnDispatch(char * Data, unsigned int Len) = 0;
};

iNetwork* CreateNetwork(const wchar_t* NetworkName = L"", size_t MaxSessionCount = 100, size_t WorkerThreadCount = 1, size_t ProcessThreadCount = 1, ePROCESS_MODE eParse = PROCESS_MODE_OWN);
