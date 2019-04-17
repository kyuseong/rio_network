#include "pch.h"
#include "DummyClient.h"
#include "../Packet/Packet.h"

#include <iostream>
#include <conio.h>

using namespace std;

DummySession::DummySession()
{
	m_Connect = true;
	m_Mode = 0;
	m_No = 0;
	
	memset(m_ProcessFunc, 0x00, sizeof(m_ProcessFunc));

	m_ProcessFunc[id_ans_chat_msg] = &DummySession::ResChatMsg;
	m_ProcessFunc[id_ans_chat_msg1] = &DummySession::ResChatMsg1;
	
}

DummySession::~DummySession()
{

}

void DummySession::OnConnect(iSessionProxy* Proxy, const wchar_t* , const int )
{
	m_Proxy = Proxy;
	ask_chat_msg SendPacket;
	SendPacket.m_No = ++m_No;
	strcpy_s(SendPacket.m_Msg, std::size(SendPacket.m_Msg), "01234567890123456789012345678901234567890123456789012345678901234567890123456789");
	m_Proxy->PostSend(reinterpret_cast<char *>(&SendPacket), SendPacket.Size);
	wprintf(L"OnConnect [IP : %s, Port : %d, SID : %d]\r\n", m_Proxy->GetPeerIP(), m_Proxy->GetPeerPort(), m_Proxy->GetSessionID());

	m_Seq = 0;
}

void DummySession::OnClose(iSessionProxy* Proxy)
{
	wprintf(L"OnClose [IP : %s, Port : %d, SID : %d]\r\n", m_Proxy->GetPeerIP(), m_Proxy->GetPeerPort(), m_Proxy->GetSessionID());
	m_Connect = false;
}

void DummySession::OnDispatch(iSessionProxy* Proxy, char *Data, unsigned int )
{
	Packet* p = (Packet*)Data;

	if (m_ProcessFunc[p->ID])
	{
		(this->*m_ProcessFunc[p->ID])(Data);
	}
}

void DummySession::ResChatMsg(char * pData)
{
	ans_chat_msg * pRecvPacket = reinterpret_cast<ans_chat_msg *>(pData);

	Assert(pRecvPacket->m_No == m_No);

	ask_chat_msg SendPacket;
	SendPacket.m_No = ++m_No;
	strcpy_s(SendPacket.m_Msg, std::size(SendPacket.m_Msg ), "01234567890123456789012345678901234567890123456789012345678901234567890123456789");
	m_Proxy->PostSend(reinterpret_cast<char *>(&SendPacket), SendPacket.Size);
}

void DummySession::ResChatMsg1(char * pData)
{
	ans_chat_msg1 * pRecvPacket = reinterpret_cast<ans_chat_msg1*>(pData);
	
	if (m_Seq != 0)
	{
		Assert(m_Seq + 1 == pRecvPacket->m_No);
	}

	m_Seq = pRecvPacket->m_No;
}
/////////////////////////////////////////////////////////////
///	DummyClient
/////////////////////////////////////////////////////////////

DummyClient::DummyClient() 
{
	m_NetworkClient = CreateNetwork( L"DummyClient", 1000, 1, 1);

	m_NetworkClient->Start([]()
	{
		return new DummySession();
	});
}

DummyClient::~DummyClient()
{
	delete m_NetworkClient;
}

bool DummyClient::Connect(const wchar_t* Address, const int Port)
{
	for (int i = 0;i < 500;i++)
	{
		auto DS = (DummySession*)m_NetworkClient->ConnectSession(Address, Port);
		if (nullptr == DS)
		{
			wprintf(L"Connect Fail [IP : %s, Port : %d]\r\n", Address, Port);
			return false;
		}
		m_ClientList.insert( std::make_pair(DS->GetSessionID(), DS) );
	}
	return true;
}

bool DummyClient::Close()
{
	for (auto& Client : m_ClientList)
	{
		m_NetworkClient->DisconnectSession(Client.second->GetSessionID());
	}
	return true;
}

void DummyClient::Shutdown()
{
	for (auto& Client : m_ClientList)
	{
		m_NetworkClient->DisconnectSession(Client.second->GetSessionID());
	}
}

