#pragma once

#include "DummySession.h"

class DummyServer
{
private:
	using NetMap = std::map<long long, DummySession *>;
	iNetworkDelegate* m_NetworkDelegate;
	iNetwork *m_Network;		///	소켓 서버
	std::mutex			m_Lock;				///	동기화를 위한 락
	NetMap				m_mapNet;			///	넷 맵
	std::atomic<long long> m_MaxNo;			///	새로 접속한 유저의 ID값 얻음

public:

	DummyServer();

	~DummyServer();

	///	서버 시작
	void	Start();

	/// 서버 종료
	void	Shutdown();

	///	유저 연결 추가
	bool	AddNet(DummySession *pNet);

	///	유저 삭제
	bool	RemoveNet(DummySession *pNet);

	///	유저 연결 종료
	void	DisconnectNet(DummySession *pNet);

	///	Account ID로 유저 소켓 얻기
	DummySession * GetPlayerNet(long long no);

	void Broadcast(void * Data, unsigned int Len);
	void Check();

	///	현재 접속 유저 수 얻기
	const unsigned short GetConnectedUserCount() const { return m_Network->GetSessionCount(); }
	

public:
	static DummyServer* GetInstance() { static DummyServer instance; return &instance; }

};

