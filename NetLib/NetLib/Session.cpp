#include "NetLibPCH.h"
#include "Network.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Buffer.h"
#include "IOBuffer.h"
#include "Buffer.h"

// 세션의 IO 모델 
class SessionIOModel
{
protected:
	Session* m_Session;
public:
	SessionIOModel(Session* Session) : m_Session(Session)
	{
	}
	virtual ~SessionIOModel()
	{
	}

	virtual bool Attach() = 0;
	virtual bool PostRecv() = 0;
	virtual bool PostSend(char *SendBuffer, int SendOffset, int SendSize) = 0;
};

// 세션의 IO 모델 (IOCP)
class SessionIOModel_IOCP : public SessionIOModel
{
	OverlappedBuffer	m_SendBuffer;	// 보내기 할때 OV
	OverlappedBuffer	m_RecvBuffer;	// 받기 할때 OV
public:
	SessionIOModel_IOCP(Session* Session) 
		: SessionIOModel(Session),
		m_SendBuffer(Session, IOTYPE_SEND),
		m_RecvBuffer(Session, IOTYPE_RECV)
	{
	}
	virtual ~SessionIOModel_IOCP()
	{
	}

	virtual bool Attach() override
	{
		return true;
	}

	virtual bool PostRecv() override
	{
		WSABUF	WsaBuf;
		WsaBuf.buf = m_Session->m_RecvIOBuffer->GetBuffPtr();
		WsaBuf.len = m_Session->m_RecvIOBuffer->GetBuffSize();

		DWORD NumBytes = 0;
		DWORD Flags = 0;

		if (SOCKET_ERROR == ::WSARecv(m_Session->m_Socket, &WsaBuf, 1, &NumBytes, &Flags, &m_RecvBuffer, NULL))
		{
			int iError = ::WSAGetLastError();
			if (iError != WSA_IO_PENDING)
			{
				return false;
			}
		}
		return true;
	}

	virtual bool PostSend(char *SendBuffer, int SendOffset, int SendSize) override
	{
		WSABUF WsaBuf;
		
		WsaBuf.buf = SendBuffer;
		WsaBuf.len = SendSize;

		DWORD NumBytes = 0;
		DWORD Flags = 0;

		if (SOCKET_ERROR == ::WSASend(m_Session->m_Socket, &WsaBuf, 1, &NumBytes, Flags, &m_SendBuffer, NULL))
		{
			int iError = ::WSAGetLastError();
			if (iError != WSA_IO_PENDING)
			{	
				return false;
			}
		}
		return true;
	}

};

// 세션의 IO 모델 (RIO)
class SessionIOModel_RIO : public SessionIOModel
{
	RIO_RQ			m_RequestQueue;		// RQ
	RIO_BUFFERID	m_RecvBufferId;		// RecvBufferID
	RIO_BUFFERID	m_SendBufferId;		// SendBufferID

	RIOBuffer* m_RecvRIOBuffer;
	RIOBuffer* m_SendRIOBuffer;
public:
	SessionIOModel_RIO(Session* Session) : SessionIOModel(Session)
	{
		m_RecvBufferId = RIO.RIORegisterBuffer(m_Session->m_RecvIOBuffer->GetBuffPtr(), m_Session->m_RecvIOBuffer->GetBuffSize());
		if (m_RecvBufferId == RIO_INVALID_BUFFERID)
		{
			throw std::exception("RIORegisterBuffer - error");
		}

		m_SendBufferId = RIO.RIORegisterBuffer(m_Session->m_SendBuffer->GetBuffPtr(), m_Session->m_SendBuffer->GetBuffSize());
		if (m_SendBufferId == RIO_INVALID_BUFFERID)
		{
			throw std::exception("RIORegisterBuffer - error");
		}

		m_RecvRIOBuffer = new RIOBuffer(m_Session, IOTYPE_RECV, m_RecvBufferId);
		m_SendRIOBuffer = new RIOBuffer(m_Session, IOTYPE_SEND, m_SendBufferId);
	}

	virtual ~SessionIOModel_RIO()
	{
		delete m_SendRIOBuffer;
		delete m_RecvRIOBuffer;

		RIO.RIODeregisterBuffer(m_RecvBufferId);
		RIO.RIODeregisterBuffer(m_SendBufferId);
	}

	virtual bool Attach() override
	{
		const int SEND_RQ_SIZE = 1;
		const int RECV_RQ_SIZE = 1;	//  For Windows 8 and Windows Server 2012 , this parameter must be 1.
		const int SEND_BUFF_COUNT = 1;
		const int RECV_BUFF_COUNT = 1;	//  For Windows 8 and Windows Server 2012 , this parameter must be 1.

		// RQ 를 생성한다.
		m_RequestQueue = RIO.RIOCreateRequestQueue(m_Session->m_Socket, SEND_RQ_SIZE, SEND_BUFF_COUNT, RECV_RQ_SIZE, RECV_BUFF_COUNT,
			(RIO_CQ)m_Session->m_Network.GetCompletionQueue(m_Session->m_ThreadId),
			(RIO_CQ)m_Session->m_Network.GetCompletionQueue(m_Session->m_ThreadId), NULL);
		if (m_RequestQueue == RIO_INVALID_RQ)
		{
			ERRORLOG("[DEBUG] RIOCreateRequestQueue Error: %d\n", GetLastError());
			return false;
		}

		return true;
	}

	virtual bool PostRecv() override
	{
		m_RecvRIOBuffer->BufferId = m_RecvBufferId;
		m_RecvRIOBuffer->Length = m_Session->m_RecvIOBuffer->GetBuffSize();
		m_RecvRIOBuffer->Offset = 0;

		DWORD Flags = 0;

		// 받기 요청
		if (false == RIO.RIOReceive(m_RequestQueue, (PRIO_BUF)m_RecvRIOBuffer, 1, Flags, m_RecvRIOBuffer))
		{
			return false;
		}

		return true;
	}

	bool PostSend(char *SendBuffer, int SendOffset, int SendSize)
	{
		m_SendRIOBuffer->BufferId = m_SendBufferId;
		m_SendRIOBuffer->Offset = static_cast<ULONG>(SendOffset);
		m_SendRIOBuffer->Length = static_cast<ULONG>(SendSize);

		DWORD Flags = 0;
		if (false == RIO.RIOSend(m_RequestQueue, (PRIO_BUF)m_SendRIOBuffer, 1, Flags, m_SendRIOBuffer))
		{
			return false;
		}

		return true;
	}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Session::Session(Network &Net, unsigned short SessionID, iSessionStub* iS, const unsigned int SendBufSize /*= SEND_SIZE*/, const unsigned int RecvBufSize /*= RECV_SIZE*/)
	: m_Network(Net)
	, m_Socket(INVALID_SOCKET)
	, m_ID(SessionID)
	, m_RefCount(0)
	
{
	m_iSession = iS;

	m_RecvBuffer = new RingBuffer(SocketUtils::GetBestIOBufferSize(MAX_PACKET_SIZE));
	m_SendBuffer = new RingBuffer(SocketUtils::GetBestIOBufferSize(SendBufSize));
	m_RecvIOBuffer = new Buffer(SocketUtils::GetBestIOBufferSize(RecvBufSize));
	
	if (nullptr == m_RecvIOBuffer || nullptr == m_RecvBuffer || nullptr == m_SendBuffer) 
	{
		throw std::exception("buff can't alloc");
	}

	if (SupportRIO)
	{
		m_IOModel = new SessionIOModel_RIO(this);
	}
	else
	{
		m_IOModel = new SessionIOModel_IOCP(this);
	}
	m_ProcOverlapped = new OverlappedBuffer(this, IOTYPE_PROCESS);

	m_PeerIP.clear();
	m_PeerPort = 0;
	m_TargetIP.clear();
	m_TargetPort = 0;

	m_Connect = false;
}

Session::~Session()
{
	delete m_IOModel;
	delete m_ProcOverlapped;

	if (m_RecvBuffer)	delete m_RecvBuffer;	m_RecvBuffer = nullptr;
	if (m_SendBuffer)	delete m_SendBuffer;	m_SendBuffer = nullptr;
	if (m_RecvIOBuffer)	delete m_RecvIOBuffer;	m_RecvIOBuffer = nullptr;
}

void Session::SetPeerIP(const wchar_t* PeerAddress, const int PeerPort)
{
	m_PeerIP = PeerAddress;
	m_PeerPort = PeerPort;
}

void Session::SetTargetIP(const wchar_t* TargetAddress, const int TargetPort)
{
	m_TargetIP = TargetAddress;
	m_TargetPort = (u_short)TargetPort;
}

void Session::SetThreadID(int ThreadID)
{
	m_ThreadId = ThreadID;
}

void Session::Attach(SOCKET hSocket)
{
	std::lock_guard Lock(m_ClientSessionLock);

	m_Socket		= hSocket;

	if (m_IOModel->Attach() == false)
	{
		NET_ASSERT(false);
		return;
	}

	{
		std::lock_guard RecvLock(m_RecvPacketLock);
		m_RecvBuffer->Clear();
	}
	{
		std::lock_guard SendLock(m_SendPacketLock);	
		m_SendBuffer->Clear();
	}
	
	m_RefCount = 0;

	AddRef(0, L"Attach", __LINE__);

	m_RecvPending.clear();
	m_SendPending.clear();
	m_ProcPending.clear();
	
	m_PeerIP.clear();
	m_TargetIP.clear();
	m_PeerPort = 0;
	m_TargetPort = 0;

	m_Connect = true;
#ifdef DEBUG_NETWORK
	m_DisconnectTime = 0;
	m_SendTime = 0;
	m_ReceiveTime = 0;
	m_ProcTime = 0;

	m_PostSendTime = 0;
	m_PostReceiveTime = 0;
	m_PostProcTime = 0;

	m_DisconnectCount = 0;
	m_SendCount = 0;
	m_PostSendCount = 0;
	m_RecvCount = 0;
	m_PostRecvCount = 0;
	m_ProcCount = 0;
	m_PostProcCount = 0;
#endif

	m_DisconnectReason = DISCONNECT_REASON_NONE;

	ERRORLOG(L"Session::Attach - sid:%d", GetSessionID());
}

// 실제로 소켓을 끊어준다.
// 이후 Ref Count가 0이 되면 모두 연결 해제 시킨다.
void Session::Disconnect(eDISCONNECT_REASON Reason, const wchar_t * SrcFile, const unsigned int SrcLine)
{
	{
		std::lock_guard Lock(m_ClientSessionLock);

		if (!IsConnected())
			return;

		if (m_DisconnectReason == DISCONNECT_REASON_NONE)
		{
			m_DisconnectReason = Reason;
		}

		LINGER lingerOption;
		lingerOption.l_onoff = 1;
		lingerOption.l_linger = 0;

		if (SOCKET_ERROR == ::setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
		{
			ERRORLOG(L"Disconnect - setsockopt linger option error: %d\n", GetLastError());
		}

		ERRORLOG(L"Disconnect - sid:%d,reasion:%d", GetSessionID(), Reason);

		::closesocket(m_Socket);


		m_Socket = INVALID_SOCKET;
		m_Connect = false;
#ifdef DEBUG_NETWORK
		m_DisconnectTime = GetTickCount();
#endif
	}
#ifdef DEBUG_NETWORK
	InterlockedIncrement(&m_DisconnectCount);
#endif
	ReleaseRef(1, SrcFile, SrcLine);
}

// Ref Count를 더해준다.
void Session::AddRef(int index, const wchar_t * SrcFile, const unsigned int SrcLine)
{ 
	long Count = InterlockedIncrement(&m_RefCount);
	UNREFERENCED_PARAMETER(Count);
#ifdef DEBUG_NETWORK
	NET_ASSERT(index < 10);
	InterlockedIncrement(&m_RefAddCount[index]);
	if (IsConnected() == false)
	{
//		ERRORLOG(L"%s(%d) : Count - Discoonnect - sid:%d, count:%d", SrcFile, SrcLine, GetSessionID(), Count);
	}
#endif
}

// Ref Count를 빼준다.
void Session::ReleaseRef(int index, const wchar_t * SrcFile, const unsigned int SrcLine)
{
#ifdef DEBUG_NETWORK
	NET_ASSERT(index < 10);
	InterlockedIncrement(&m_RefReleaseCount[index]);
#endif
	int RefCount = InterlockedDecrement(&m_RefCount);
	if (0 == RefCount)
	{
		// 끝났음
		m_Network.AddClosingSession(this, __FILEW__, __LINE__);
	}
#ifdef DEBUG_NETWORK
	if (IsConnected() == false)
	{
		ERRORLOG(L"%s(%d) : ReleaseRef - Discoonnect - sid:%d, count:%d", SrcFile, SrcLine, GetSessionID(), RefCount);
	}

	if (RefCount < 0)
	{
		ERRORLOG(L"%s(%d) : ReleaseRef - sid:%d, count:%d", SrcFile, SrcLine, GetSessionID(), RefCount);
	}
#endif
	NET_ASSERT(RefCount >= 0);
}

// 접속이 종료된다.
void Session::Close(const wchar_t * SrcFile, const unsigned int SrcLine)
{
	ERRORLOG(L"Session::Close - sid:%d", GetSessionID());

	OnClose();

	std::lock_guard Lock(m_ClientSessionLock);

	// 0이어야 한다.
	NET_ASSERT(m_RefCount == 0);
	NET_ASSERT(m_Socket == INVALID_SOCKET);
	NET_ASSERT(m_Connect == false);

	{
		std::lock_guard RecvLock(m_RecvPacketLock);
		m_RecvBuffer->Clear();
	}

	{
		std::lock_guard SendLock(m_SendPacketLock);
		m_SendBuffer->Clear();
	}

	m_PeerIP.clear();
	m_TargetIP.clear();

	m_PeerPort = 0;
	m_TargetPort = 0;
}

void Session::PostRecv()
{
	// recv 한번만할수 있도록 해준다.
	if (m_RecvPending.try_lock() == false)
		return;

	m_ClientSessionLock.lock();
	if (!IsConnected()) 
	{
		m_ClientSessionLock.unlock();

		m_RecvPending.unlock();
		return;
	}
	// 
	AddRef(2, L"PostRecv", __LINE__);
	
	if (m_IOModel->PostRecv() == false)
	{
		m_ClientSessionLock.unlock();
		m_RecvPending.unlock();

		Disconnect(DISCONNECT_REASON_RECV_POSTRECV_ERROR, __WFILE__, __LINE__);
		ReleaseRef(3, L"PostRecv", __LINE__);
		return;
	}

#ifdef DEBUG_NETWORK
	InterlockedIncrement(&m_PostRecvCount);
	m_PostReceiveTime = GetTickCount();
#endif

	m_ClientSessionLock.unlock();
}

void Session::CompletedRecv(unsigned int Transferred)
{
#ifdef DEBUG_NETWORK
	InterlockedIncrement(&m_RecvCount);
#endif

	if(Transferred > 0)
	{
		std::lock_guard Lock(m_RecvPacketLock);

		if (m_RecvBuffer->PutData(m_RecvIOBuffer->GetBuffPtr(), Transferred) == false) {
			ERRORLOG(L"Socket::Recevie() - PutData Error - %u \n", GetSessionID());
			Disconnect(DISCONNECT_REASON_RECV_PUTDATA_ERROR, __WFILE__, __LINE__);
		}

		m_RecvPending.unlock();
	}
	else
	{
		m_RecvPending.unlock();

		Disconnect(DISCONNECT_REASON_REMOTE_CLOSED, __WFILE__, __LINE__);
	}
#ifdef DEBUG_NETWORK
	m_ReceiveTime = GetTickCount();
#endif
}

// 언마샬링 raw
char* Session::UnmarshallRaw(char * Data, unsigned int &Len)
{
	std::lock_guard Lock(m_RecvPacketLock);
	
	return Unmarshall(Data, Len, m_RecvBuffer);
}

// 패킷이 완료되었나?
bool Session::IsCompletedPacket(unsigned int &Len, RingBuffer* RecvBuffer)
{
	if (0 == RecvBuffer->GetDataLength())
		return false;

	// 임의로 헤더 2바이트로
	if (RecvBuffer->GetDataLength() < 2)
		return false;

	// 패킷 전체 사이즈 ( header 포함 + payload )
	unsigned short PacketSize = 0;
	RecvBuffer->GetData((char*)&PacketSize, 2);

	// 패킷이 다 받아 졌는지 체크
	if (RecvBuffer->GetDataLength() < PacketSize)
		return false;

	Len = PacketSize;

	return true;
}

// 언마샬링
char* Session::Unmarshall(char * Data, unsigned int &Len, RingBuffer* RecvBuffer)
{
	// 패킷 완료되었어?
	if (IsCompletedPacket(Len, RecvBuffer) == false)
		return nullptr;

	// 패킷 최대 사이즈을 넘어가면 안됨
	if (Len >= MAX_PACKET_SIZE)
	{
		throw std::runtime_error("packet size exceeded maximum size.");
		ERRORLOG(L"Session::Unmarshall() - packet size exceeded maximum size - %u, %d \n", GetSessionID(), Len);
		return nullptr;
	}

	// 패킷 전체 사이즈 만큼 Data 에 복사(header 포함되어 있음)
	if (RecvBuffer->GetData(Data, Len) == false)
	{
		ERRORLOG(L"Session::Unmarshall() - GetData() - %u \n", GetSessionID());
		throw std::runtime_error("There is not enough data in the buffer.");
		return nullptr;
	}

	RecvBuffer->PopData(Len);

	return Data;
}

///////////////////////////////////////////////////////////////////////////////
// 패킷 마샬링
///////////////////////////////////////////////////////////////////////////////
bool Session::Marshall(void* Data, unsigned int Len, RingBuffer* SendBuffer)
{
	if (false == IsConnected()) {
		return false;
	}
	
	if (SendBuffer->PutData((char*)Data, Len) == false) {
		ERRORLOG(L"Marshal - PutData fail - %d", GetSessionID());
		return false;
	}
	
	return true;
}

// send 처리 ( 보내기 하지 않음 / 버퍼에만 넣음 )
bool Session::PostSendBuffered(void* Data, unsigned int Len)
{
	{
		std::lock_guard Lock(m_SendPacketLock);
		if (false == Marshall(Data, Len, m_SendBuffer)) {
			return false;
		}
	}

	return true;
}

// send 처리 ( 버퍼에 넣고 flush 함 )
bool Session::PostSend(void * Data, unsigned int Len)
{
	if (nullptr == Data)
	{
		return PostSend();
	}

	{
		std::lock_guard Lock(m_SendPacketLock);
		if (false == Marshall(Data, Len, m_SendBuffer)) {
			return false;
		}
		if (m_SendBuffer->GetDataLength() != (int)Len)
		{
			return true;
		}
	}
	
	bool bRet = PostSend();
	return bRet;
}

// send 완료
bool Session::CompletedSend(unsigned int Transferred)
{
#ifdef DEBUG_NETWORK
	InterlockedIncrement(&m_SendCount);
#endif
	if (!IsConnected())
	{
		ERRORLOG(L"CompletedSend - error - ip %s , socket id %u , send buffer length %u , send completed %u", GetTargetIP(), GetSessionID(), m_SendBuffer->GetDataLength(), Transferred);

		m_SendPending.unlock();

		return false;
	}

	if (Transferred == 0)
	{
		m_SendPending.unlock();
		Disconnect(DISCONNECT_REASON_REMOTE_CLOSED, __WFILE__, __LINE__);
		return false;
	}

#ifdef DEBUG_NETWORK
	m_SendTime = GetTickCount();
#endif
	m_SendPacketLock.lock();

	if (Transferred > (unsigned int)m_SendBuffer->GetDataLength())
	{
		m_SendBuffer->PopData(m_SendBuffer->GetDataLength());

		m_SendPacketLock.unlock();

		m_SendPending.unlock();

		Disconnect(DISCONNECT_REASON_SEND_POPDATA_ERROR, __WFILE__, __LINE__);

		return false;
	}

	m_SendBuffer->PopData(Transferred);

	m_SendPacketLock.unlock();

	m_SendPending.unlock();

	bool bRet = PostSend();

	return bRet;
}

// flush 하기
bool Session::PostSend()
{
	if (false == IsConnected())
		return false;

	int SendOffset = 0;
	int SendSize = 0;
	char* SendBuffer = nullptr;

	{
		std::lock_guard Lock(m_SendPacketLock);
		unsigned int Len = m_SendBuffer->GetDataLength();
		if (Len == 0)
		{
			return true;
		}
		SendOffset = m_SendBuffer->GetFirstDataOffset();
		SendSize = m_SendBuffer->GetFirstDataLen();
		SendBuffer = m_SendBuffer->GetFirstData();
	}

	// pending 된 상태면 pass
	if (m_SendPending.try_lock() == false)
		return true;

	m_ClientSessionLock.lock();

	if (false == IsConnected())
	{
		m_ClientSessionLock.unlock();

		m_SendPending.unlock();
		return false;
	}

	AddRef(4, L"PostSend", __LINE__);

	if (false == m_IOModel->PostSend(SendBuffer, SendOffset, SendSize))
	{
		m_ClientSessionLock.unlock();

		m_SendPending.unlock();
		
		Disconnect(DISCONNECT_REASON_SEND_POSTSEND_ERROR, __WFILE__, __LINE__);
		ReleaseRef(5, L"PostSend", __LINE__);

		ERRORLOG(L"PostSend Error - sid:%d,offset:%d,size:%d", GetSessionID(), SendOffset, SendSize);

		return false;
	}

#ifdef DEBUG_NETWORK
	InterlockedIncrement(&m_PostSendCount);
	m_PostSendTime = GetTickCount();
#endif

	m_ClientSessionLock.unlock();

	return true;
}

void Session::PostProcess()
{
	{
		std::lock_guard Lock(m_RecvPacketLock);
		unsigned int Len = 0;
		if (IsCompletedPacket(Len, m_RecvBuffer) == false)
			return;
	}

	if (m_ProcPending.try_lock() == false)
		return;

	{
		std::lock_guard Lock(m_ClientSessionLock);
		if (false == IsConnected())
		{
			m_ProcPending.unlock();
			return;
		}

#ifdef DEBUG_NETWORK
		m_PostProcTime = GetTickCount();
#endif
		AddRef(6, L"PostProcess", __LINE__);
#ifdef DEBUG_NETWORK
		InterlockedIncrement(&m_PostProcCount);
#endif
	}
	m_Network.EnqueueProcess((ULONG_PTR)this, 0, m_ProcOverlapped);
}

void Session::ProcessCompletion()
{
#ifdef DEBUG_NETWORK
	InterlockedIncrement(&m_ProcCount);
	m_ProcTime = GetTickCount();
#endif
	
	char Buffer[MAX_PACKET_SIZE];
	unsigned int Len = 0;
	bool HasError = false;
	try
	{
		char* Packet = nullptr;
		while ((Packet = UnmarshallRaw(Buffer, Len)) != nullptr)
		{
			OnDispatch(Packet, Len);
		}
	}
	catch (std::exception& )
	{
		HasError = true;
	}
	
	m_ProcPending.unlock();

	if (HasError)
	{
		Disconnect(DISCONNECT_REASON_SEND_POSTSEND_ERROR, __WFILE__, __LINE__);
	}

	// 남이 있는거 있으면 한번더~
	PostProcess();
}

int Session::ProcessPacket(char * Buffer, unsigned int & Len)
{
	if (m_Network.GetProcessMode() != PROCESS_MODE_USER)
		return -1;

	int Count = 0;

	bool HasError = false;

	try
	{
		char* Packet = nullptr;
		while ((Packet = UnmarshallRaw(Buffer, Len)) != nullptr)
		{
			OnDispatch(Packet, Len);
			Count++;
		}
	}
	catch (std::exception&)
	{
		HasError = true;
	}

	if (HasError)
	{
		Disconnect(DISCONNECT_REASON_SEND_POSTSEND_ERROR, __WFILE__, __LINE__);
	}

	return Count;
}

int Session::ProcessPacket()
{
	char Buffer[MAX_PACKET_SIZE];
	unsigned int Len = 0;

	int Ret = ProcessPacket(Buffer, Len);
	return Ret;
}

