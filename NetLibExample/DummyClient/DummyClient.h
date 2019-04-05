#pragma once


#include "Network.h"

class ClientNet : public Session
{
public:
	bool			m_Connect;
	short			m_Mode;
	int				m_No;
	std::mutex		m_Lock;
	std::wstring	m_UserName;

	int				m_Seq = 0;
private:
	typedef void (ClientNet::*ProcessFunc)(char * pData);
	ProcessFunc	m_ProcessFunc[10];

public:
	ClientNet(Network &net, unsigned short ID);
	virtual ~ClientNet();

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
	std::mutex		m_Lock;
	iNetwork* m_NetworkClient;		///	����
	std::vector<Session*> m_ClientList;		///	����

private:
	std::wstring	m_IP;				///	IP
	unsigned int	m_Port;				///	��Ʈ
public:
	DummyClient();
	~DummyClient();

	static DummyClient* GetInstance() { static DummyClient instance; return &instance; }
	
	///	 ������ ����
	bool	ConnectServer();
	/// ����
	bool	CloseChatServer();

	///		���� ����
	void	Shutdown();
	

};

#define g_DummyClient	DummyClient::GetInstance()