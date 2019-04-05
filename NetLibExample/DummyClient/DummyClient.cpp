#include "pch.h"
#include "DummyClient.h"
#include "../Packet/Packet.h"

#include <iostream>
#include <conio.h>

using namespace std;

ClientNet::ClientNet(Network &net, unsigned short ID) : Session(net, ID)
{
	m_Connect = true;
	m_Mode = 0;
	m_No = 0;
	
	memset(m_ProcessFunc, 0x00, sizeof(m_ProcessFunc));

	m_ProcessFunc[id_ans_chat_msg] = &ClientNet::ResChatMsg;
	m_ProcessFunc[id_ans_chat_msg1] = &ClientNet::ResChatMsg1;
	
}

ClientNet::~ClientNet()
{

}

void ClientNet::OnConnect(const wchar_t* , const int )
{
	ask_chat_msg SendPacket;
	SendPacket.m_No = ++m_No;
	strcpy_s(SendPacket.m_Msg, std::size(SendPacket.m_Msg), "01234567890123456789012345678901234567890123456789012345678901234567890123456789");
	PostSend(reinterpret_cast<char *>(&SendPacket), SendPacket.Size);
	wprintf(L"OnConnect [IP : %s, Port : %d, SID : %d]\r\n", this->GetPeerIP(), GetPeerPort(), GetSessionID());

	m_Seq = 0;
}

void ClientNet::OnClose()
{
	wprintf(L"OnClose [IP : %s, Port : %d, SID : %d]\r\n", this->GetPeerIP(), GetPeerPort(), GetSessionID());

	m_Connect = false;
}

void ClientNet::OnDispatch(char *Data, unsigned int )
{
	Packet* p = (Packet*)Data;

	if (m_ProcessFunc[p->ID])
	{
		(this->*m_ProcessFunc[p->ID])(Data);
	}
}

void ClientNet::ResChatMsg(char * pData)
{
	ans_chat_msg * pRecvPacket = reinterpret_cast<ans_chat_msg *>(pData);

	Assert(pRecvPacket->m_No == m_No);

	ask_chat_msg SendPacket;
	SendPacket.m_No = ++m_No;
	strcpy_s(SendPacket.m_Msg, std::size(SendPacket.m_Msg ), "01234567890123456789012345678901234567890123456789012345678901234567890123456789");
	PostSend(reinterpret_cast<char *>(&SendPacket), SendPacket.Size);
}

void ClientNet::ResChatMsg1(char * pData)
{
	ans_chat_msg1 * pRecvPacket = reinterpret_cast<ans_chat_msg1*>(pData);
	
	if (m_Seq != 0)
	{
//		Assert(m_Seq + 1 == pRecvPacket->m_RoomNo);
	}

	m_Seq = pRecvPacket->m_No;
}
/////////////////////////////////////////////////////////////
///	DummyClient
/////////////////////////////////////////////////////////////

DummyClient::DummyClient() 
{
	m_NetworkClient = CreateNetwork( L"DummyClient", 1000, 1, 1);

	m_NetworkClient->Start([](Network& Net, unsigned short SID)
	{
		return new ClientNet(Net, SID);
	});

	m_IP = L"127.0.0.1";
	m_Port = 10031;
}

DummyClient::~DummyClient()
{
	delete m_NetworkClient;
}

bool DummyClient::ConnectServer()
{
	for (int i = 0;i < 500;i++)
	{
		auto ClientNet = m_NetworkClient->ConnectSession(m_IP.c_str(), m_Port);
		if (nullptr == ClientNet)
		{
			wprintf(L"Connect Fail [IP : %s, Port : %d]\r\n", m_IP.c_str(), m_Port);
			return false;
		}
		m_ClientList.push_back(ClientNet);
	}
	return true;
}

bool DummyClient::CloseChatServer()
{
	for (auto Client : m_ClientList)
	{
		Client->Disconnect(DISCONNECT_REASON_HOST_CLOSED, __WFILE__, __LINE__);
	}
	return true;
}

void DummyClient::Shutdown()
{
	for (auto& Client : m_ClientList)
		Client->Disconnect( DISCONNECT_REASON_SHUTDOWN_CLOSE, __FILEW__, __LINE__);
}

