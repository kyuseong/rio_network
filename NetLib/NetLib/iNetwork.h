#pragma once 

#include "TypesCommon.h"

#include "Allocator.h"
#include "Network.h"
#include "Mutex.h"
#include "AssertEx.h"
#include "Output.h"


// ��Ŷ ó�� ������ �����ϱ�
enum ePROCESS_MODE : DWORD
{
	PROCESS_MODE_USER,		// ��Ŷ ó���� ������ �ٸ� �����忡�� ���ܰ��� ó���Ҳ���
	PROCESS_MODE_OWN		// ��Ŷ ó���� �� proc ������� �̿� �Ҳ���
};

using CreateClientSessionFunc = std::function<Session* (Network&, unsigned short)>;

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
	virtual Session * ConnectSession(const wchar_t* TargetAddress, const int TargetPort) = 0;
	// Ŭ���̾�Ʈ�� ���� ������ ������ ���´�.
	virtual void DisconnectSession(Session * Session) = 0;

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

class iNetworkDelegate
{
public:
	virtual void OnAccept(const wchar_t* TargetAddress, const int TargetPort) = 0;
	virtual void OnConnect(const wchar_t* TargetAddress, const int TargetPort) = 0;
	virtual void OnClose() = 0;
	virtual void OnDispatch(char * Data, unsigned int Len) = 0;
};

iNetwork* CreateNetwork(const wchar_t* NetworkName = L"", size_t MaxSessionCount = 100, size_t WorkerThreadCount = 1, size_t ProcessThreadCount = 1, ePROCESS_MODE eParse = PROCESS_MODE_OWN);
