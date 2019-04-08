#include "pch.h"

#include "DummyServer.h"
#include "DummySession.h"
#include "../Packet/Packet.h"

#include <iostream>

DummyServer::DummyServer() 
{
	m_MaxNo = 0;
		
	m_Network = CreateNetwork(L"DummyServer", 10000, 2, 2);
	
	m_Network->Start([](auto& Net, auto ID){
		return new DummySession(Net, ID);
	});
}

DummyServer::~DummyServer()
{
}

void DummyServer::Start(const wchar_t* Address, int Port)
{
	m_Network->StartAcceptor(Address, Port, true);
}

void DummyServer::Shutdown()
{
	m_Network->Shutdown();
}

bool DummyServer::AddNet(DummySession *pNet)
{
	std::lock_guard Lock(m_Lock);
	pNet->SetNo(++m_MaxNo);

	if (false == m_mapNet.insert(NetMap::value_type(pNet->GetNo(), pNet)).second)
	{
		ERRORLOG(L"ERROR - AddNet - sid:%d, no:%d, count:%d", pNet->GetSessionID(),  pNet->GetNo(), m_mapNet.size());


		return false;
	}
	ERRORLOG(L"AddNet - sid:%I64d, no:%d, count:%d", pNet->GetSessionID(), pNet->GetNo(), m_mapNet.size());

	return true;
}

bool DummyServer::RemoveNet(DummySession *pNet)
{
	std::lock_guard Lock(m_Lock);

	auto Itr = m_mapNet.find(pNet->GetNo());
	if (Itr == m_mapNet.end())
	{
		ERRORLOG(L"ERROR - sid:%d, no:%I64d, count:%d", pNet->GetSessionID(), pNet->GetNo(), m_mapNet.size());
	}
	else
	{
		m_mapNet.erase(pNet->GetNo());
		ERRORLOG(L"RemoveNet- sid:%d, no:%I64d, count:%d", pNet->GetSessionID(), pNet->GetNo(), m_mapNet.size());
	}

	return true;
}

void DummyServer::Broadcast(void * Data, unsigned int Len)
{
	std::lock_guard Lock(m_Lock);

	for (auto& Pair : m_mapNet)
	{
		Pair.second->PostSend(Data, Len);
	}
}

void DummyServer::Check()
{
	std::lock_guard Lock(m_Lock);

	for (auto& Pair : m_mapNet)
	{
		Pair.second->Check();
	}
}


void DummyServer::DisconnectNet(DummySession * pNet)
{
	m_Network->DisconnectSession(pNet);
}

DummySession * DummyServer::GetPlayerNet(const long long no)
{
	std::lock_guard Lock(m_Lock);

	auto iter = m_mapNet.find(no);
	if (iter == m_mapNet.end())
		return nullptr;
	return iter->second;
}
