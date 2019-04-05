#pragma once

#include "SocketConfig.h"

class CryptPacket;
class Buffer;
class RingBuffer;
class RIOBuffer;
class SessionIOModel;
class OverlappedBuffer;


enum : unsigned int 
{
	MAX_PACKET_SIZE = 1024 * 64,	/// 패킷 최대 사이즈
	SEND_SIZE = 1024 * 64, 
	RECV_SIZE = 1024 * 64 
};

// 접속이 종료된 이유
enum eDISCONNECT_REASON 
{
	DISCONNECT_REASON_NONE,
	DISCONNECT_REASON_HOST_CLOSED,					// disconnect 을 호출해서 끊었어
	DISCONNECT_REASON_RECV_POSTRECV_ERROR,			// post recv 호출했을때 에러 났엉
	DISCONNECT_REASON_SEND_POSTSEND_ERROR,			// post send 호출했을때 에러 났엉
	DISCONNECT_REASON_RECV_PUTDATA_ERROR,			// 데이터를 너무 많이 받았엉
	DISCONNECT_REASON_SEND_POPDATA_ERROR,			// 보낸거 보다 많이 보냈다고 하네?
	DISCONNECT_REASON_REMOTE_CLOSED,				// 클라에서 접속이 끊어짐
	DISCONNECT_REASON_PROC_POSTPROCESS_ERROR,		// 데이터 패킷 처리 요청 에러
	DISCONNECT_REASON_SHUTDOWN_CLOSE				// 서버 종료
};

// 접속 세션
class Session
{
	friend class Network;
	friend class SessionIOModel_RIO;
	friend class SessionIOModel_IOCP;

	Network			&m_Network;			// 
	bool			m_Connect;			// 접속 유무
	SOCKET			m_Socket;			// 소켓 핸들
	const int		m_ID;				// 소캣 고유 아이디
	
	eDISCONNECT_REASON m_DisconnectReason;	// 접속 해제된 이유

	Buffer*			m_RecvIOBuffer;		// wsarecv 에 걸리는 buffer
	RingBuffer*		m_RecvBuffer;		// recv 한 패킷들 처리하기까지 보관하는 buffer
	RingBuffer*		m_SendBuffer;		// 보내기 위해서 저장해 두는 buffer

	SessionIOModel* m_IOModel;
	int				m_ThreadId;			// 할당된 스레드 id

	OverlappedBuffer* m_ProcOverlapped;	// 패킷 처리 요청할때 OV
	SpinLock		m_ClientSessionLock;// 세션 전체 락 (접속 / 해제시 초기화)
	SpinLock		m_RecvPacketLock;	// RecvPacketBuffer에 대한 락
	SpinLock		m_SendPacketLock;	// SendPacketBuffer에 대한 락

	std::wstring	m_PeerIP;			// 클라이언트 IP
	unsigned short	m_PeerPort;			// 클라이언트 PORT
	std::wstring	m_TargetIP;			// 서버 IP의 주소
	unsigned short	m_TargetPort;		// 서버 IP의 PORT

	volatile long	m_RefCount;			// 참조 카운터
	SpinLock		m_RecvPending;		// 받기를 한번만 하게 해주기 위해서
	SpinLock		m_SendPending;		// 보내기를 한번만 하게 해주기 위해서
	SpinLock		m_ProcPending;		// 처리를 한번만 하게 해주기 위해서

#ifdef DEBUG_NETWORK
	volatile long	m_RefAddCount[10]{};	// 참조 카운터
	volatile long	m_RefReleaseCount[10]{};// 참조 카운터
	volatile long	m_DisconnectCount;	// 참조 카운터
	volatile long	m_SendCount;		// 참조 카운터
	volatile long	m_PostSendCount;	// 참조 카운터
	volatile long	m_RecvCount;		// 참조 카운터
	volatile long	m_PostRecvCount;	// 참조 카운터
	volatile long	m_ProcCount;		// 참조 카운터
	volatile long	m_PostProcCount;	// 참조 카운터

	DWORD			m_PostSendTime;
	DWORD			m_PostReceiveTime;
	DWORD			m_PostProcTime;
	DWORD			m_DisconnectTime;
	DWORD			m_SendTime;
	DWORD			m_ReceiveTime;
	DWORD			m_ProcTime;
#endif
public:
	Session(Network &Net, unsigned short SessionID, const unsigned int SendBufSize = SEND_SIZE, const unsigned int RecvBufSize = RECV_SIZE);
	virtual ~Session();
	Session(const Session & rhs) = delete;
	Session& operator=(Session & rhs) = delete;

	// 참고 하게 되면 레퍼런스를 올림
	void AddRef(int index, const wchar_t * SrcFile, const unsigned int SrcLine);
	// 해제 하게 되면 레퍼런스를 내림
	void ReleaseRef(int index, const wchar_t * SrcFile, const unsigned int SrcLine);

	//----------------------------------------------------------------------------
	// IO RECV 
	//----------------------------------------------------------------------------
	// Recv 을 요청한다.
	void PostRecv();
	// 리시브가 완료되었다.
	void CompletedRecv(unsigned int Transferred);
	// 쌓여있는 패킷을 처리한다.
	void ProcessReceivedData(char * pData, unsigned int & Len);

	//----------------------------------------------------------------------------
	// IO SEND
	//----------------------------------------------------------------------------
	// Send을 요청한다.
	bool PostSend(void* Data, unsigned int Len);
	// 버퍼에 채움
	bool PostSendBuffered(void* Data, unsigned int Len);
	// Send버퍼를 Flush 시킨다.
	bool PostSend(); 
	// Send가 완료되었다.
	bool CompletedSend(unsigned int Transferred);
	//----------------------------------------------------------------------------
	// 패킷 처리
	// 처리 옵션이 PROCESS_MODE_OWN 인경우 동작
	//----------------------------------------------------------------------------
	// 처리 요청
	void PostProcess();
	// 처리 완료
	void ProcessCompletion();
	
	// 패킷 마샬링
	virtual bool Marshall(void* Data, unsigned int Len, RingBuffer* SendBuffer);
	// 패킷 언마샬링 실제
	char* UnmarshallRaw(char * Data, unsigned int &Len);
	// 패킷 언마살링
	virtual char* Unmarshall(char * Data, unsigned int &Len, RingBuffer* RecvBuffer);
	
	//----------------------------------------------------------------------------
	// 접속 종료
	//----------------------------------------------------------------------------
	// 서버에서 접속을 종료 시킨다.
	// 강제로 이 함수를 호출 해도 정상 동작한다.
	// 실제로 Ref Count 가 0이 되면 이 세션은 끊어진다. 
	// (Close함수가 호출 되고 OnClose을 통해 처리 할수 있게된다)
	void Disconnect(eDISCONNECT_REASON Reason, const wchar_t * SrcFile, const unsigned int SrcLine);
private:
	// 실제로 종료처리가 된다.
	void Close(const wchar_t * SrcFile/* = __WFILE__*/, const unsigned int SrcLine/* = __LINE__*/);
	
	//----------------------------------------------------------------------------
	// 이 부분을 구현해서 처리하세요~
	//----------------------------------------------------------------------------
protected:
	virtual void OnAccept(const wchar_t*, const int) = 0;
	virtual void OnConnect(const wchar_t* TargetAddress, const int TargetPort) = 0;
	virtual void OnClose() = 0;
	virtual void OnDispatch(char * Data, unsigned int Len) = 0;

	//----------------------------------------------------------------------------
	// Misc
	//----------------------------------------------------------------------------
public:
	virtual bool IsConnected() { return m_Socket != INVALID_SOCKET && m_Connect; }
	// 소켓 핸들을 구하기
	SOCKET GetSocket() const { return m_Socket; }
	// 고유 번호 구하기
	int GetSessionID() const { return m_ID; }
	// 원격 클라이언트의 접속 ip 구하기
	const wchar_t*	GetPeerIP()		const { return m_PeerIP.c_str(); }
	// 원격 클라이언트의 접속 포트 구하기
	const unsigned short GetPeerPort() const { return m_PeerPort; }
	// 접속 할 서버의 IP (클라이언트인 경우)
	const wchar_t* GetTargetIP() { return m_TargetIP.c_str(); }
	// 접속 할 서버의 PORT (클라이언트인 경우)
	const unsigned short GetTargetPort() const { return m_TargetPort; }

	void SetThreadID(int ThreadID);
	int GetThreadID() const { return m_ThreadId; }

private:
	void SetPeerIP(const wchar_t* PeerAddress, const int PeerPort);
	void SetTargetIP(const wchar_t* TargetAddress, const int TargetPort);
	void Attach(SOCKET hSocket);
};
