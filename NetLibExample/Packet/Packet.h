#pragma once

#pragma pack(push, 1)

enum Packet_ID : unsigned char
{
	id_ask_chat_msg = 1,
	id_ans_chat_msg = 2,
	id_ans_chat_msg1 = 3,
};
	
struct Packet
{
	unsigned short Size;
	unsigned char ID;
};

struct ask_chat_msg : Packet
{
	ask_chat_msg()
	{
		Size = sizeof(ask_chat_msg);
		ID = id_ask_chat_msg;
	}
	char m_Msg[100];
	int m_No;
};

struct ans_chat_msg : Packet
{
	ans_chat_msg()
	{
		Size = sizeof(ask_chat_msg);
		ID = id_ans_chat_msg;
	}
	char m_Msg[100];
	int m_No;
};

struct ans_chat_msg1 : Packet
{
	ans_chat_msg1()
	{
		Size = sizeof(ans_chat_msg1);
		ID = id_ans_chat_msg1;
	}
	char m_Msg[100];
	int m_No;

};
	

#pragma pack(pop)
