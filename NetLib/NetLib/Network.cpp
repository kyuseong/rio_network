#include "NetLibPCH.h"
#include "SocketUtils.h"
#include "IOCompletionQueue.h"
#include "RIOBuffer.h"
#include "Output.h"
#include "Network.h"
#include "iNetwork.h"


// 네트워크 - IO Model 
class NetworkIOModel
{
protected:
	Network* m_Network;
public:
	NetworkIOModel(Network* Net) : m_Network(Net) {}

	virtual ~NetworkIOModel() {}
	virtual bool Start(size_t IoThreadCount, size_t MaxSessionCount) = 0;
	virtual int RunIOWorker(unsigned int ThreadId) = 0;
	virtual void* GetCompletionQueue(unsigned int ThreadId) const = 0;
	virtual void Associate(HANDLE Socket , Session * Session) = 0;
	virtual SOCKET CreateSocket() = 0;
	virtual void Shutdown() = 0;
};

// 네트워크 - IO Model (IOCP)
class NetworkIOModel_IOCP : public NetworkIOModel
{
	IOCompletionPort*	m_IOCP;
	size_t	 m_IoThreadCount = 0;
public:
	NetworkIOModel_IOCP(Network* Net) : NetworkIOModel(Net)
	{
		m_IOCP = new IOCompletionPort(0);
	}

	virtual ~NetworkIOModel_IOCP()
	{
		if (m_IOCP)
		{
			delete m_IOCP;
			m_IOCP = nullptr;
		}
	}

	virtual bool Start(size_t IoThreadCount, size_t MaxSessionCount) override
	{
		m_IoThreadCount = IoThreadCount;
		UNREFERENCED_PARAMETER(MaxSessionCount);

		return true;
	}

	virtual void Shutdown()  override
	{
		for (unsigned int i = 0;i < m_IoThreadCount;++i)
			EnqueueIO(0, 0, 0);
	}

	virtual void Associate(HANDLE Socket, Session * Session) override
	{
		m_IOCP->AssociateDevice(Socket, (ULONG_PTR)Session);
	}

	void* GetCompletionQueue(unsigned int ThreadId) const
	{
		return m_IOCP->GetHandle(ThreadId);
	}
	
	bool DequeueIO(Session *&Session, DWORD &Transfer, WSAOVERLAPPED *&pOvl, DWORD &dwLastError) const
	{
		return m_IOCP->GetCompletionStatus((PDWORD_PTR)&Session, &Transfer, &pOvl, &dwLastError);
	}

	void EnqueueIO(ULONG_PTR completionKey, DWORD dwNumBytes, OVERLAPPED *pOverlapped) const
	{
		m_IOCP->PostStatus(completionKey, dwNumBytes, pOverlapped);
	}

	virtual SOCKET CreateSocket() override
	{
		SOCKET socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

		const int rcvlen = 0, sndlen = 0;

		::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char *)&rcvlen, sizeof(rcvlen));
		::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char *)&sndlen, sizeof(sndlen));

		return socket;
	}
	
	virtual int RunIOWorker(unsigned int ThreadId) override
	{
		while (m_Network->m_Running)
		{
			DWORD Transfer = 0;
			Session *Session = NULL;
			OverlappedBuffer	*pOvl = NULL;
			DWORD dwLastError = 0;
			if (!DequeueIO(Session, Transfer, (WSAOVERLAPPED*&)pOvl, dwLastError))
			{
				if (Session) {
					Session->Disconnect(DISCONNECT_REASON_REMOTE_CLOSED, __WFILE__, __LINE__);
					Session->ReleaseRef(6, __WFILE__, __LINE__);
				}
				continue;
			}

			if (!Session) {
				ERRORLOG(L"[RunIOWorker] shutdown - thread : %u\n", std::this_thread::get_id());
				break;
			}

			switch (pOvl->m_IOType)
			{
			case IO_RECV:
			{
				Session->CompletedRecv(Transfer);
				switch (m_Network->GetProcessMode())
				{
				case PROCESS_MODE_USER:
					Session->PostRecv();				//	recv 만 받고 끝. 실제 Parsing() 을 호출하는 코드를 원하는 곳(메인루프)에서 작성해야함.
					break;
				case PROCESS_MODE_OWN:
					Session->PostProcess();
					Session->PostRecv();
					break;
				}
			}
			break;
			case IO_SEND:
			{
				Session->CompletedSend(Transfer);
			}
			break;
			default:
				break;
			}

			Session->ReleaseRef(6, __WFILE__, __LINE__);
		}

		return 0;
	}
};

// 네트워크 - IO Model (RIO)
class NetworkIOModel_RIO : public NetworkIOModel
{
	RIOCompletionQueue* m_RioCompletionQueue;	// rio cq 

public:
	NetworkIOModel_RIO(Network* Net) : NetworkIOModel(Net)
	{
		m_RioCompletionQueue = nullptr;
	}

	~NetworkIOModel_RIO()
	{
		if (m_RioCompletionQueue)
		{
			delete m_RioCompletionQueue;
		}
	}

	bool Start(size_t IoThreadCount, size_t MaxSessionCount)
	{
		m_RioCompletionQueue = new RIOCompletionQueue((unsigned int)IoThreadCount, (unsigned int)(MaxSessionCount * 2));

		return true;
	}

	virtual void Shutdown() override
	{
	}

	virtual void Associate(HANDLE Socket, Session * Session) override
	{
	}

	void* GetCompletionQueue(unsigned int ThreadId) const
	{
		return m_RioCompletionQueue->GetHandle(ThreadId);
	}
	
	virtual SOCKET CreateSocket() override
	{
		SOCKET socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO);
		return socket;
	}


	virtual int RunIOWorker(unsigned int ThreadId) override
	{
		//::SetThreadDescription(GetCurrentThread(), (m_NetworkName + L"_IoWorkerThread").c_str());

		RIOResultSet results;

		while (m_Network->m_Running)
		{

			ULONG numResults = RIO.RIODequeueCompletion( (RIO_CQ)m_RioCompletionQueue->GetHandle(ThreadId), results.GetData(), results.GetSize());
			if (0 == numResults || RIO_CORRUPT_CQ == numResults)
			{
				if (RIO_CORRUPT_CQ == numResults)
				{
					break;
				}
				else
				{
					Sleep(1);
					continue;
				}
			}

			for (ULONG i = 0; i < numResults; ++i)
			{
				RIOBuffer* Context = results.GetRIOBuffer(i);
				ULONG Transferred = results.GetBytesTransferred(i);

				Session* ClientSession = Context->m_ClientSession;

				if (IO_RECV == Context->GetIOType())
				{
					ClientSession->CompletedRecv(Transferred);

					switch (m_Network->GetProcessMode())
					{
					case PROCESS_MODE_USER:
						ClientSession->PostRecv();
						break;
					case PROCESS_MODE_OWN:
						ClientSession->PostProcess();
						ClientSession->PostRecv();
						break;
					}
				}
				else if (IO_SEND == Context->GetIOType())
				{
					ClientSession->CompletedSend(Transferred);
				}

				if (IO_RECV == Context->GetIOType())
				{
					ClientSession->ReleaseRef(6, L"RunIOWorker", __LINE__);
				}
				else
				{
					ClientSession->ReleaseRef(7, L"RunIOWorker", __LINE__);
				}
			}
			results.Update(numResults);
		}

		return 0;
	}
};


Network::Network(const wchar_t* NetworkName, size_t MaxSessionCount, size_t WorkerThreadCount, size_t ProcessThreadCount,  ePROCESS_MODE eParse)
	: m_NetworkName(NetworkName)
	, m_IoThreadCount(WorkerThreadCount)
	, m_ProcessThreadCount(ProcessThreadCount)
	, m_MaxSessionCount(MaxSessionCount)
	, m_ProcessMode(eParse)
	, m_ActiveSessionList(nullptr)
	, m_ListenSocket(INVALID_SOCKET)
	, m_SessionCountPerThread(nullptr)
	
{
	SocketUtils::Startup();

	if (SupportRIO)
	{
		m_IOModel = std::make_unique<NetworkIOModel_RIO>(this);
	}
	else
	{
		m_IOModel = std::make_unique<NetworkIOModel_IOCP>(this);
	}

	m_ProcessIOCP = std::make_unique<IOCompletionPort>(0);
}

Network::~Network()
{
	Shutdown();

	if (m_SessionCountPerThread) {
		delete[] m_SessionCountPerThread;
		m_SessionCountPerThread = nullptr;
	}

	m_ClosingSessionList.clear();
	
	SocketUtils::Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
// io thread 와 process thread 을 시작한다.
///////////////////////////////////////////////////////////////////////////////
void Network::StartThread()
{
	m_Running = true;

	for (size_t i = 0; i < m_IoThreadCount; ++i) {
		m_IOThreads.emplace_back( std::make_unique<std::thread>([this, ThreadID = i]() -> int
			{
				return this->IoWorkerThreadEntry((unsigned int)ThreadID);
			})
		);
	}
	m_ClosingThread = std::make_unique<std::thread>([this]() -> int
	{
		return this->CloseingThreadEntry();
	});

	switch (GetProcessMode())
	{
	case PROCESS_MODE_USER:
	{

	}
	break;
	case PROCESS_MODE_OWN:
	{
		for (size_t i = 0; i < m_ProcessThreadCount; ++i)
		{
			m_ProcessThreads.emplace_back(std::make_unique<std::thread>([this]() -> int
			{
				return this->ProcessWorkerThreadEntry();
			})
			);
		}
	}
	break;
	}
}


void Network::Start(CreateClientSessionFunc CreateFunction)
{
	if (m_IoThreadCount == 0) {
		m_IoThreadCount = SocketUtils::GetNumberOfProcessors();
	}
	if (m_ProcessThreadCount == 0) {
		m_ProcessThreadCount = SocketUtils::GetNumberOfProcessors();
	}

	CreateSessionPool(CreateFunction);

	if (m_IOModel->Start( m_IoThreadCount, m_MaxSessionCount) == false)
	{
		ERRORLOG(L"Network::Start - IOModel 초기화 실패 - : %s\r\n", GetLastErrorMessage(::GetLastError()).c_str());
		throw std::exception("Network::Start - IOModel 초기화 실패" );
	}

	if (m_SessionCountPerThread) {
		delete[] m_SessionCountPerThread;
		m_SessionCountPerThread = nullptr;
	}

	m_SessionCountPerThread = new int[m_IoThreadCount] {};

	StartThread();
}

void Network::Shutdown()
{
	m_Running = false;

	StopAcceptor();

	m_IOModel->Shutdown();
	
	for (int i = 0;i < m_ProcessThreads.size();++i)
		EnqueueProcess(0, 0, 0);

	for (auto& Thread : m_IOThreads)
	{
		Thread->join();
	}
	m_IOThreads.clear();

	for (auto& Thread : m_ProcessThreads)
	{
		Thread->join();
	}
	m_ProcessThreads.clear();

	// 접속 중인 세션 접속 끊기
	{
		std::lock_guard lock(m_SessionListLock);
		for (int i = 0; i < m_MaxSessionCount; ++i)
		{
			Session* Socket = m_ActiveSessionList[i];
			if (Socket) {
				DisconnectSession(Socket);
			}
		}
	}

	// 세션들 모두 접속이 끊어질때까지 기다림
	while (false == WaitForClosedSession())
	{
		Sleep(1000);
	}
	if (m_ClosingThread)
	{
		// closing thread 셧다운
		m_ClosingSessionCondition.notify_all();
		// 대기
		m_ClosingThread->join();

		m_ClosingThread = nullptr;
	}

	// 세션 모드 제거하기
	DestroySessionPool();
}

void*	Network::GetCompletionQueue(unsigned int ThreadId) const
{
	return m_IOModel->GetCompletionQueue(ThreadId);
}


bool Network::DequeueProcess(Session *&Session, DWORD &Transfer, WSAOVERLAPPED *&pOvl, DWORD &dwLastError) const
{
	return m_ProcessIOCP->GetCompletionStatus((PDWORD_PTR)&Session, &Transfer, &pOvl, &dwLastError);
}

void Network::EnqueueProcess(ULONG_PTR completionKey, DWORD dwNumBytes, OVERLAPPED *pOverlapped) const
{
	m_ProcessIOCP->PostStatus(completionKey, dwNumBytes, pOverlapped);
}

void Network::DisconnectSession(Session * Session)
{
	if (!Session) {
		return;
	}

	Session->Disconnect(DISCONNECT_REASON_HOST_CLOSED, __WFILE__, __LINE__ );
}

void Network::AddClosingSession(Session * Session, const wchar_t * SrcFile, const unsigned int SrcLine)
{
	if (!Session) {
		Assert(Session != nullptr);
		return;
	}

	{
		std::unique_lock lock(m_ClosingSessionListLock);

		Assert(m_ClosingSessionList.end() == std::find(m_ClosingSessionList.begin(), m_ClosingSessionList.end(), Session));

		m_ClosingSessionList.emplace_back(Session);
	}

	m_ClosingSessionCondition.notify_one();
}

int Network::IoWorkerThreadEntry(unsigned int ThreadID)
{
	return m_IOModel->RunIOWorker(ThreadID);
}

int Network::ProcessWorkerThreadEntry()
{
	//::SetThreadDescription(GetCurrentThread(), (m_NetworkName + L"_ProcessWorkerThread").c_str());
	

	while (m_Running) {
		DWORD Transfer = 0;
		Session *Session = NULL;
		OverlappedBuffer *OverlappedBuffer = NULL;
		DWORD LastError = 0;

		if (!DequeueProcess(Session, Transfer, (OVERLAPPED*&)OverlappedBuffer, LastError)) {
			// error
			if (Session != NULL)
			{
				Session->Disconnect(DISCONNECT_REASON_PROC_POSTPROCESS_ERROR, __WFILE__, __LINE__);

				Session->ReleaseRef(8, L"ProcessWorkerThreadEntry", __LINE__);
			}
			continue;
		}

		if (!Session) {
			break;
		}

		switch (OverlappedBuffer->GetIOType())
		{
		case IO_PROCESS:
			try {
				Session->ProcessCompletion();
				break;
			}
			catch (const std::exception e) {
				ERRORLOG(L"ProcessCompletion error");
			}
		default:
			Assert(false);
			ERRORLOG(L"ProcessWorkerThreadEntry error");
			break;
		}

		Session->ReleaseRef(9, L"ProcessWorkerThreadEntry", __LINE__);
	}
	
	
	return 0;
}

int Network::CloseingThreadEntry()
{
	for (;;)
	{
		Session* Session = nullptr;
		{
			std::unique_lock<std::mutex> lock(m_ClosingSessionListLock);
			m_ClosingSessionCondition.wait(lock,
				[this] { return m_Running == false || !m_ClosingSessionList.empty(); });

			if (m_Running == false && m_ClosingSessionList.empty())
				break;

			Session = std::move(this->m_ClosingSessionList.front());
			this->m_ClosingSessionList.pop_front();
		}

		if (Session)
		{
			ReleaseSession(Session, __WFILE__, __LINE__);
		}
	}

	return 0;
}

Session* Network::AllocateSession(SOCKET hSocket)
{
	std::lock_guard lock(m_SessionListLock);

	if (m_FreeSessionList.empty()) {
		return nullptr;
	}

	Session* Session = m_FreeSessionList.front();
	m_FreeSessionList.pop_front();

	if (Session == nullptr) {
		return nullptr;
	}

	m_ActiveSessionList[Session->GetSessionID()] = Session;

	int ThreadId = GetProperThreadIDAndIncCount();
	Session->SetThreadID((int)ThreadId);

	Session->Attach(hSocket);

	m_IOModel->Associate(reinterpret_cast<HANDLE>(hSocket), Session);
	
	++m_SessionCount;

	return Session;
}

void Network::ReleaseSession(Session * Session, const wchar_t * SrcFile, const unsigned int SrcLine)
{
	if (!Session) {
		Assert(Session != nullptr);
		return;
	}

	Session->Close(SrcFile, SrcLine);

	std::lock_guard lock(m_SessionListLock);
	Assert(m_ActiveSessionList[Session->GetSessionID()] == Session);
	if (m_ActiveSessionList[Session->GetSessionID()] != Session)
		return;

	m_ActiveSessionList[Session->GetSessionID()] = nullptr;
	m_FreeSessionList.push_back(Session);
	--m_SessionCount;

	DecSessionCountPerThread(Session);
}

///////////////////////////////////////////////////////////////////////////////
// 세션 풀 만들기
///////////////////////////////////////////////////////////////////////////////
void Network::CreateSessionPool(CreateClientSessionFunc CreateFunction)
{
	m_SessionCount = 0;
	m_ActiveSessionList = new Session*[m_MaxSessionCount];
	for (unsigned short i = 0; i < m_MaxSessionCount; ++i) {
		m_ActiveSessionList[i] = nullptr;
	}

	// 세션 생성하기
	for (unsigned short i = 0; i < m_MaxSessionCount; ++i)
	{
		try {
			iSessionStub* iSess = CreateFunction();

			Session* Sess= new Session(*this, i, iSess);

			PushFreeSocket(Sess);
		}
		catch (const std::bad_alloc &) {
			ERRORLOG(L"Network::CreateSessionPool - new fail Error : %s\r\n", GetLastErrorMessage(::GetLastError()).c_str());
			throw std::exception("Network::CreateSessionPool - new fail", ::GetLastError());
		}
	}

}

///////////////////////////////////////////////////////////////////////////////
// 세션 풀 해제하기
///////////////////////////////////////////////////////////////////////////////
void Network::DestroySessionPool()
{
	std::lock_guard lock(m_SessionListLock);

	if (nullptr != m_ActiveSessionList)
	{
		for (int i = 0; i < m_MaxSessionCount; ++i) {
			if (m_ActiveSessionList[i]) {
				delete m_ActiveSessionList[i];
				m_ActiveSessionList[i] = NULL;
			}
		}
	}
	for (auto Session : m_FreeSessionList)
	{
		delete Session;
	}
	m_FreeSessionList.clear();

	if (m_ActiveSessionList) {
		delete[] m_ActiveSessionList;
		m_ActiveSessionList = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
// 소켓이 다 끊어질때까지 기다리기
///////////////////////////////////////////////////////////////////////////////
bool Network::WaitForClosedSession()
{
	for (int i = 0; i < m_MaxSessionCount; ++i)
	{
		std::lock_guard lock(m_SessionListLock);
		if (m_ActiveSessionList[i] != NULL)
			return false;
	}
	return true;
}


void Network::PushFreeSocket(Session * Session)
{
	std::lock_guard lock(m_SessionListLock);

	m_FreeSessionList.push_back(Session);
}

bool Network::IsConnected(unsigned short Index)
{
	if (Index >= m_MaxSessionCount)	return false;
	
	std::lock_guard lock(m_SessionListLock);
	auto Session = m_ActiveSessionList[Index];
	return Session ? Session->IsConnected() : false;
}


int Network::GetProperThreadIDAndIncCount()
{
	auto* MinThread = std::min_element(&m_SessionCountPerThread[0], &m_SessionCountPerThread[m_IoThreadCount]);
	__int64 ThreadId = __int64(MinThread - m_SessionCountPerThread);
	++m_SessionCountPerThread[ThreadId];

	return (int)ThreadId;
}

void Network::DecSessionCountPerThread(Session * Session)
{
	auto ThreadID = Session->GetThreadID();
	--m_SessionCountPerThread[ThreadID];
}

SOCKET Network::CreateListenSocket(const wchar_t* Address, unsigned short port, bool NoDelay)
{
	SOCKET socket = m_IOModel->CreateSocket();
	if (socket == INVALID_SOCKET) {
		ERRORLOG(L"Network::CreateListenSocket() - Error : %s\r\n", GetLastErrorMessage(::WSAGetLastError()).c_str());
		return INVALID_SOCKET;
	}

	do {
		const int opt = 1;
		const int rcvlen = 0, sndlen = 0;
		if (SOCKET_ERROR == ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))) {
			break;
		}

		if (NoDelay)
		{
			if (SOCKET_ERROR == ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt))) {
				break;
			}
		}
		std::string address = ws2s(Address);
		if (address.empty()) {
			address = "0.0.0.0";
		}
		sockaddr_in	addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(address.c_str());

		if (SOCKET_ERROR == ::bind(socket, reinterpret_cast<struct sockaddr *>(const_cast<SOCKADDR_IN*>(&addr)), sizeof(SOCKADDR_IN))) {
			break;
		}

		if (SOCKET_ERROR == ::listen(socket, SOMAXCONN)) {
			break;
		}

		return socket;

	} while (false);

	::closesocket(socket);
	return INVALID_SOCKET;
}

void Network::StartAcceptor(const wchar_t* ListenAddr, int ListenPort, bool NoDelay)
{
	m_ListenAddress = ListenAddr;
	m_ListenPort = ListenPort;
	if (m_ListenSocket == INVALID_SOCKET)
	{
		m_ListenSocket = CreateListenSocket(ListenAddr, ListenPort, NoDelay);
	}

	if (m_Acceptor == nullptr)
	{
		m_Acceptor = std::make_unique<std::thread>([this]()
		{
			this->AcceptThread();
		});
	}
}

void Network::StopAcceptor()
{
	if (m_ListenSocket != INVALID_SOCKET)
	{
		::closesocket(m_ListenSocket);
		m_ListenSocket = INVALID_SOCKET;
	}

	if (m_Acceptor)
	{
		m_Acceptor->join();
		m_Acceptor = nullptr;
	}
}

int Network::AcceptThread()
{
	//::SetThreadDescription(GetCurrentThread(), (m_NetworkName + L"_AcceptThread").c_str());
	while (m_Running)
	{
		// accept
		SOCKET AcceptedSock = accept(m_ListenSocket, NULL, NULL);
		if (AcceptedSock == INVALID_SOCKET) {
			ERRORLOG(L"Error : %s", GetLastErrorMessage(::WSAGetLastError()).c_str());
			continue;
		}

		// 원격 ip 주소를 얻어온다.
		SOCKADDR_IN PeerAddr;
		int AddrLen = sizeof(PeerAddr);
		getpeername(AcceptedSock, (SOCKADDR*)&PeerAddr, &AddrLen);
		std::wstring Addr = s2ws(inet_ntoa(PeerAddr.sin_addr));
		u_short Port = ntohs(PeerAddr.sin_port);

		Session * Session = AllocateSession(AcceptedSock);
		if (nullptr == Session) {
			::closesocket(AcceptedSock);
			continue;
		}
		Session->SetPeerIP(Addr.c_str(), Port);
		Session->OnAccept(Addr.c_str(), Port);
		Session->PostRecv();
	}

	return 0;
}

iSessionStub * Network::ConnectSession(const wchar_t* TargetAddress, const int TargetPort)
{
	SOCKET Socket = m_IOModel->CreateSocket();

	if (Socket == INVALID_SOCKET) {
		ERRORLOG(L"Network::ConnectSession() - 소켓 생성 실패: ip %s, port %u Error : %s\r\n", TargetAddress, TargetPort, GetLastErrorMessage(::WSAGetLastError()).c_str());
		return nullptr;
	}

	struct sockaddr_in addr {};
	addr.sin_family = AF_INET;
	std::string address = ws2s(TargetAddress);

	addr.sin_addr.s_addr = inet_addr(address.c_str());
	addr.sin_port = htons((u_short)TargetPort);

	if (SOCKET_ERROR == ::connect(Socket, reinterpret_cast<struct sockaddr *>(const_cast<SOCKADDR_IN*>(&addr)), sizeof(SOCKADDR_IN)))
	{
		int LastError = WSAGetLastError();
		if (WSAEWOULDBLOCK != LastError)
		{
			ERRORLOG(L"Network::ConnectSession() - connect 실패 - ip:%s, port:%u Error:%s\r\n", TargetAddress, TargetPort, GetLastErrorMessage(::WSAGetLastError()).c_str());
			return nullptr;
		}
	}

	// 원격 ip 주소를 얻어온다.
	SOCKADDR_IN PeerAddr{};
	int AddrLen = sizeof(PeerAddr);
	getpeername(Socket, (SOCKADDR*)&PeerAddr, &AddrLen);
	std::wstring Addr = s2ws(inet_ntoa(PeerAddr.sin_addr));
	u_short Port = ntohs(PeerAddr.sin_port);

	Session * Session = AllocateSession(Socket);
	
	if (nullptr == Session)
	{
		return nullptr;
	}
	
	Session->SetTargetIP(TargetAddress, TargetPort);
	Session->SetPeerIP(Addr.c_str(), Port);
	Session->OnConnect(TargetAddress, TargetPort);
	Session->PostRecv();

	return Session->GetiSession();
}


void Network::DisconnectSession(int SessionID)
{
	auto Sess = GetSession(SessionID);
	DisconnectSession(Sess);
}

iNetwork* CreateNetwork(const wchar_t* NetworkName, size_t MaxSessionCount, size_t WorkerThreadCount, size_t ProcessThreadCount, ePROCESS_MODE eParse)
{
	Network* Net = new Network
	(
		NetworkName,
		MaxSessionCount,
		WorkerThreadCount,
		ProcessThreadCount,
		eParse
	);
	return Net;
}
