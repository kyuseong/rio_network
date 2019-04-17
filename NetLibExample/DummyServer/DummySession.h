#pragma once

class DummySession : public iSessionStub
{
private:
	UINT64 m_PingReceivedTime = 0;					///	�� ���� �ð�
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
	// ���� Ŭ���̾�Ʈ�� ���� ip ���ϱ�
	const wchar_t*	GetPeerIP() const
	{
		return m_Proxy->GetPeerIP();
	}
	// ���� Ŭ���̾�Ʈ�� ���� ��Ʈ ���ϱ�
	const unsigned short GetPeerPort() const
	{
		return m_Proxy->GetPeerPort();
	}
	// ���� �� ������ IP (Ŭ���̾�Ʈ�� ���)
	const wchar_t* GetTargetIP() const 
	{
		return m_Proxy->GetTargetIP();
	}
	// ���� �� ������ PORT (Ŭ���̾�Ʈ�� ���)
	const unsigned short GetTargetPort() const
	{
		return m_Proxy->GetTargetPort();
	}

	// Send�� ��û�Ѵ�.
	bool PostSend(void* Data, unsigned int Len)
	{
		return m_Proxy->PostSend(Data, Len);
	}
};

