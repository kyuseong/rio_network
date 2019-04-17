#pragma once

class DummySession : public iSessionStub
{
private:
	UINT64 m_PingReceivedTime = 0;					///	핑 받은 시간
	size_t m_LastSequence = 0;

	iSessionProxy* m_Proxy = nullptr;
	long long m_No = 0;

public:
	explicit DummySession();
	virtual ~DummySession();

	virtual void OnAccept(iSessionProxy* Proxy, const wchar_t*, const int) override;
	virtual void OnConnect(iSessionProxy*, const wchar_t*, const int) override {}
	virtual void OnClose(iSessionProxy* Proxy) override;
	virtual void OnDispatch(iSessionProxy* Proxy, char * Data, unsigned int Len) override;

	const UINT64	GetPingReceivedTime() const { return m_PingReceivedTime; }

	size_t			GetLastSequence() const { return m_LastSequence; }
	void			SetLastSequence(size_t value) { m_LastSequence = value; }

	void			ChatMsg(char * Data);
	void Check();

	long long GetNo() {
		Assert(m_No != 0);
		return m_No;
	}
	void SetNo(long long No)
	{
		m_No = No;
		Assert(m_No != 0);
	}

	int GetSessionID() const
	{
		return m_Proxy->GetSessionID();
	}
	// 원격 클라이언트의 접속 ip 구하기
	const wchar_t*	GetPeerIP() const
	{
		return m_Proxy->GetPeerIP();
	}
	// 원격 클라이언트의 접속 포트 구하기
	const unsigned short GetPeerPort() const
	{
		return m_Proxy->GetPeerPort();
	}
	// 접속 할 서버의 IP (클라이언트인 경우)
	const wchar_t* GetTargetIP() const 
	{
		return m_Proxy->GetTargetIP();
	}
	// 접속 할 서버의 PORT (클라이언트인 경우)
	const unsigned short GetTargetPort() const
	{
		return m_Proxy->GetTargetPort();
	}

	// Send을 요청한다.
	bool PostSend(void* Data, unsigned int Len)
	{
		return m_Proxy->PostSend(Data, Len);
	}
};

