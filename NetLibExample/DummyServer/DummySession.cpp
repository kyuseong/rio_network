#include "pch.h"

#include "DummySession.h"
#include "DummyServer.h"
#include "../Packet/Packet.h"

DummySession::DummySession(Network &Net, unsigned short ID) : Session(Net, ID, 1024 * 64, 1024 * 64)
{
}

DummySession::~DummySession()
{
}

void DummySession::OnAccept(const wchar_t*, const int)
{
	m_PingReceivedTime = GetTickCount64();

	DummyServer::GetInstance()->AddNet(this);
}

void DummySession::OnClose()
{
	SetLastSequence(0);
	
	DummyServer::GetInstance()->RemoveNet(this);

	m_PingReceivedTime = 0;
}

void DummySession::OnDispatch(char * Data, unsigned int )
{
	Packet* p = (Packet*)Data;

	switch (p->ID)
	{
	case id_ask_chat_msg:
		ChatMsg(Data);
		break;
	default:
		break;
	}
}

std::atomic<int> g_Count = 0;

void DummySession::ChatMsg(char * Data)
{
	ask_chat_msg * pRecvPacket = reinterpret_cast<ask_chat_msg*>(Data);

	ans_chat_msg SendPacket;
	SendPacket.m_No = pRecvPacket->m_No;
	memcpy(SendPacket.m_Msg, pRecvPacket->m_Msg, sizeof SendPacket.m_Msg);

	PostSend(&SendPacket, SendPacket.Size);

	g_Count++;

	m_PingReceivedTime = GetTickCount64();
}


void DummySession::Check()
{
	if (IsConnected() == false)
		return;
	auto Current = GetTickCount64();
	Assert(Current - m_PingReceivedTime < 1000);
}
