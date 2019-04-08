#pragma once


#include "Network.h"

class DummySession : public Session
{
public:
	bool m_Connect;
	short m_Mode;
	int m_No;
	std::mutex m_Lock;
	std::wstring	m_UserName;
	int m_Seq = 0;
private:
	typedef void (DummySession::*ProcessFunc)(char * pData);
	ProcessFunc	m_ProcessFunc[10];

public:
	DummySession(Network &net, unsigned short ID);
	virtual ~DummySession();

	virtual void OnAccept(const wchar_t*, const int) {}
	virtual void OnConnect(const wchar_t* , const int );
	virtual void OnClose();
	virtual void OnDispatch(char * pData, unsigned int len);

	void ResChatMsg(char * pData);
	void ResChatMsg1(char * pData);
};

class DummyClient
{
public:
	std::mutex m_Lock;
	iNetwork* m_NetworkClient;
	std::vector<Session*> m_ClientList;		///	����

public:
	DummyClient();
	~DummyClient();

	static DummyClient* GetInstance() { static DummyClient instance; return &instance; }
	
	///	 ������ ����
	bool	Connect(const wchar_t* Address, const int Port);
	// ����
	bool	Close();

	// ���� ����
	void	Shutdown();
	

};

#define g_DummyClient	DummyClient::GetInstance()