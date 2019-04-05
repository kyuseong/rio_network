#pragma once

#include "DummySession.h"

class DummyServer
{
private:
	using NetMap = std::map<long long, DummySession *>;
	iNetworkDelegate* m_NetworkDelegate;
	iNetwork *m_Network;		///	���� ����
	std::mutex			m_Lock;				///	����ȭ�� ���� ��
	NetMap				m_mapNet;			///	�� ��
	std::atomic<long long> m_MaxNo;			///	���� ������ ������ ID�� ����

public:

	DummyServer();

	~DummyServer();

	///	���� ����
	void	Start();

	/// ���� ����
	void	Shutdown();

	///	���� ���� �߰�
	bool	AddNet(DummySession *pNet);

	///	���� ����
	bool	RemoveNet(DummySession *pNet);

	///	���� ���� ����
	void	DisconnectNet(DummySession *pNet);

	///	Account ID�� ���� ���� ���
	DummySession * GetPlayerNet(long long no);

	void Broadcast(void * Data, unsigned int Len);
	void Check();

	///	���� ���� ���� �� ���
	const unsigned short GetConnectedUserCount() const { return m_Network->GetSessionCount(); }
	

public:
	static DummyServer* GetInstance() { static DummyServer instance; return &instance; }

};

