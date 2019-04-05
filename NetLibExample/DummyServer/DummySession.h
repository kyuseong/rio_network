#pragma once


class DummySession : public Session
{
private:
	UINT64			m_PingReceivedTime = 0;					///	핑 받은 시간
	size_t			m_LastSequence = 0;

	long long m_No = 0;

public:
	explicit DummySession(Network &Net, unsigned short ID);
	virtual ~DummySession();

	virtual void OnAccept(const wchar_t*, const int) override;
	virtual void OnConnect(const wchar_t*, const int) override {}
	virtual void OnClose() override;
	virtual void OnDispatch(char * Data, unsigned int Len) override;

	const UINT64	GetPingReceivedTime() const { return m_PingReceivedTime; }

	size_t			GetLastSequence() const { return m_LastSequence; }
	void			SetLastSequence(size_t value) { m_LastSequence = value; }

	void			ChatMsg(char * Data);
	void Check();

	long long GetNo(){ 
		Assert(m_No != 0);
		return m_No; }
	void SetNo(long long No) 
	{
		m_No = No; 
		Assert(m_No != 0);

	}


};

