#pragma once

#include "SocketConfig.h"

class CryptPacket;
class Buffer;
class RingBuffer;
class RIOBuffer;
class SessionIOModel;
class OverlappedBuffer;
class iSessionStub;

enum : unsigned int 
{
	MAX_PACKET_SIZE = 1024 * 64,	/// ��Ŷ �ִ� ������
	SEND_SIZE = 1024 * 64, 
	RECV_SIZE = 1024 * 64 
};

// ������ ����� ����
enum eDISCONNECT_REASON 
{
	DISCONNECT_REASON_NONE,
	DISCONNECT_REASON_HOST_CLOSED,					// disconnect �� ȣ���ؼ� ������
	DISCONNECT_REASON_RECV_POSTRECV_ERROR,			// post recv ȣ�������� ���� ����
	DISCONNECT_REASON_SEND_POSTSEND_ERROR,			// post send ȣ�������� ���� ����
	DISCONNECT_REASON_RECV_PUTDATA_ERROR,			// �����͸� �ʹ� ���� �޾Ҿ�
	DISCONNECT_REASON_SEND_POPDATA_ERROR,			// ������ ���� ���� ���´ٰ� �ϳ�?
	DISCONNECT_REASON_REMOTE_CLOSED,				// Ŭ�󿡼� ������ ������
	DISCONNECT_REASON_PROC_POSTPROCESS_ERROR,		// ������ ��Ŷ ó�� ��û ����
	DISCONNECT_REASON_SHUTDOWN_CLOSE				// ���� ����
};

// ���� ����
class Session : public iSessionProxy
{
	friend class Network;
	friend class SessionIOModel_RIO;
	friend class SessionIOModel_IOCP;

	Network			&m_Network;			// 
	bool			m_Connect;			// ���� ����
	SOCKET			m_Socket;			// ���� �ڵ�
	const int		m_ID;				// ��Ĺ ���� ���̵�
	
	iSessionStub*		m_iSession;

	eDISCONNECT_REASON m_DisconnectReason;	// ���� ������ ����

	Buffer*			m_RecvIOBuffer;		// wsarecv �� �ɸ��� buffer
	RingBuffer*		m_RecvBuffer;		// recv �� ��Ŷ�� ó���ϱ���� �����ϴ� buffer
	RingBuffer*		m_SendBuffer;		// ������ ���ؼ� ������ �δ� buffer

	SessionIOModel* m_IOModel;
	int				m_ThreadId;			// �Ҵ�� ������ id

	OverlappedBuffer* m_ProcOverlapped;	// ��Ŷ ó�� ��û�Ҷ� OV
	SpinLock		m_ClientSessionLock;// ���� ��ü �� (���� / ������ �ʱ�ȭ)
	SpinLock		m_RecvPacketLock;	// RecvPacketBuffer�� ���� ��
	SpinLock		m_SendPacketLock;	// SendPacketBuffer�� ���� ��

	std::wstring	m_PeerIP;			// Ŭ���̾�Ʈ IP
	unsigned short	m_PeerPort;			// Ŭ���̾�Ʈ PORT
	std::wstring	m_TargetIP;			// ���� IP�� �ּ�
	unsigned short	m_TargetPort;		// ���� IP�� PORT

	volatile long	m_RefCount;			// ���� ī����
	SpinLock		m_RecvPending;		// �ޱ⸦ �ѹ��� �ϰ� ���ֱ� ���ؼ�
	SpinLock		m_SendPending;		// �����⸦ �ѹ��� �ϰ� ���ֱ� ���ؼ�
	SpinLock		m_ProcPending;		// ó���� �ѹ��� �ϰ� ���ֱ� ���ؼ�

#ifdef DEBUG_NETWORK
	volatile long	m_RefAddCount[10]{};	// ���� ī����
	volatile long	m_RefReleaseCount[10]{};// ���� ī����
	volatile long	m_DisconnectCount;	// ���� ī����
	volatile long	m_SendCount;		// ���� ī����
	volatile long	m_PostSendCount;	// ���� ī����
	volatile long	m_RecvCount;		// ���� ī����
	volatile long	m_PostRecvCount;	// ���� ī����
	volatile long	m_ProcCount;		// ���� ī����
	volatile long	m_PostProcCount;	// ���� ī����

	DWORD			m_PostSendTime;
	DWORD			m_PostReceiveTime;
	DWORD			m_PostProcTime;
	DWORD			m_DisconnectTime;
	DWORD			m_SendTime;
	DWORD			m_ReceiveTime;
	DWORD			m_ProcTime;
#endif
public:
	Session(Network &Net, unsigned short SessionID, iSessionStub* SS, const unsigned int SendBufSize = SEND_SIZE, const unsigned int RecvBufSize = RECV_SIZE);
	virtual ~Session();
	Session(const Session & rhs) = delete;
	Session& operator=(Session & rhs) = delete;

	// ���� �ϰ� �Ǹ� ���۷����� �ø�
	void AddRef(int index, const wchar_t * SrcFile, const unsigned int SrcLine);
	// ���� �ϰ� �Ǹ� ���۷����� ����
	void ReleaseRef(int index, const wchar_t * SrcFile, const unsigned int SrcLine);

	//----------------------------------------------------------------------------
	// IO RECV 
	//----------------------------------------------------------------------------
	// Recv �� ��û�Ѵ�.
	void PostRecv();
	// ���ú갡 �Ϸ�Ǿ���.
	void CompletedRecv(unsigned int Transferred);
	// �׿��ִ� ��Ŷ�� ó���Ѵ�.
	void ProcessReceivedData(char * pData, unsigned int & Len);

	//----------------------------------------------------------------------------
	// IO SEND
	//----------------------------------------------------------------------------
	// Send�� ��û�Ѵ�.
	bool PostSend(void* Data, unsigned int Len);
	// ���ۿ� ä��
	bool PostSendBuffered(void* Data, unsigned int Len);
	// Send���۸� Flush ��Ų��.
	bool PostSend(); 
	// Send�� �Ϸ�Ǿ���.
	bool CompletedSend(unsigned int Transferred);
	//----------------------------------------------------------------------------
	// ��Ŷ ó��
	// ó�� �ɼ��� PROCESS_MODE_OWN �ΰ�� ����
	//----------------------------------------------------------------------------
	// ó�� ��û
	void PostProcess();
	// ó�� �Ϸ�
	void ProcessCompletion();
	
	// ��Ŷ ������
	virtual bool Marshall(void* Data, unsigned int Len, RingBuffer* SendBuffer);
	// ��Ŷ �𸶼��� ����
	char* UnmarshallRaw(char * Data, unsigned int &Len);
	// ��Ŷ �𸶻층
	virtual char* Unmarshall(char * Data, unsigned int &Len, RingBuffer* RecvBuffer);
	
	//----------------------------------------------------------------------------
	// ���� ����
	//----------------------------------------------------------------------------
	// �������� ������ ���� ��Ų��.
	// ������ �� �Լ��� ȣ�� �ص� ���� �����Ѵ�.
	// ������ Ref Count �� 0�� �Ǹ� �� ������ ��������. 
	// (Close�Լ��� ȣ�� �ǰ� OnClose�� ���� ó�� �Ҽ� �ְԵȴ�)
	void Disconnect(eDISCONNECT_REASON Reason, const wchar_t * SrcFile, const unsigned int SrcLine);
private:
	// ������ ����ó���� �ȴ�.
	void Close(const wchar_t * SrcFile/* = __WFILE__*/, const unsigned int SrcLine/* = __LINE__*/);
	
	//----------------------------------------------------------------------------
	// �� �κ��� �����ؼ� ó���ϼ���~
	//----------------------------------------------------------------------------
protected:
	void OnAccept(const wchar_t* TargetAddress, const int TargetPort)
	{
		m_iSession->OnAccept(this, TargetAddress, TargetPort);
	}
	void OnConnect(const wchar_t* TargetAddress, const int TargetPort)
	{
		m_iSession->OnConnect(this, TargetAddress, TargetPort);
	}
	void OnClose()
	{
		m_iSession->OnClose(this);
	}
	void OnDispatch(char * Data, unsigned int Len)
	{
		m_iSession->OnDispatch(this, Data, Len);
	}

	//----------------------------------------------------------------------------
	// Misc
	//----------------------------------------------------------------------------
public:
	virtual bool IsConnected() const override { return m_Socket != INVALID_SOCKET && m_Connect; }
	// ���� �ڵ��� ���ϱ�
	SOCKET GetSocket() const { return m_Socket; }
	// ���� ��ȣ ���ϱ�
	virtual int GetSessionID() const override { return m_ID; }
	// ���� Ŭ���̾�Ʈ�� ���� ip ���ϱ�
	virtual const wchar_t*	GetPeerIP()		const override { return m_PeerIP.c_str(); }
	// ���� Ŭ���̾�Ʈ�� ���� ��Ʈ ���ϱ�
	virtual const unsigned short GetPeerPort() const override { return m_PeerPort; }
	// ���� �� ������ IP (Ŭ���̾�Ʈ�� ���)
	virtual const wchar_t* GetTargetIP() const override { return m_TargetIP.c_str(); }
	// ���� �� ������ PORT (Ŭ���̾�Ʈ�� ���)
	virtual const unsigned short GetTargetPort() const override { return m_TargetPort; }

	void SetThreadID(int ThreadID);
	int GetThreadID() const { return m_ThreadId; }

	iSessionStub* GetiSession()
	{
		return m_iSession;
	}
private:
	void SetPeerIP(const wchar_t* PeerAddress, const int PeerPort);
	void SetTargetIP(const wchar_t* TargetAddress, const int TargetPort);
	void Attach(SOCKET hSocket);
};
