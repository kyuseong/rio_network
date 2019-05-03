#pragma once

#include "SocketConfig.h"

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_PROCESS
};

class Session;

class RIOBuffer : public RIO_BUF
{
public:
	Session* m_ClientSession;
	IOType	m_IOType;
	RIO_BUFFERID m_BufferId;

	RIOBuffer(Session* Session, IOType IOType, RIO_BUFFERID BufferId)
		: m_ClientSession(Session), m_IOType(IOType), m_BufferId(BufferId)
	{
	}

	IOType GetIOType() { return m_IOType; }
};


class OverlappedBuffer : public WSAOVERLAPPED
{
public:
	Session* m_ClientSession;
	IOType	m_IOType;

	OverlappedBuffer(Session* Session, IOType IOType)
		: m_ClientSession(Session), m_IOType(IOType)
	{
		hEvent = NULL;
	}

	IOType GetIOType() { return m_IOType; }
};