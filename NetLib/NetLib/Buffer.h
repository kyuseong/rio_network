#pragma once

// �� ����
class Buffer final
{
protected:
	char	*m_Buffer;		// ����
	int		m_BufSize;		// ���� ������

public:
	Buffer(int Size = 4096)
	{
		m_Buffer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		m_BufSize = Size;
	}

	virtual ~Buffer()
	{
		VirtualFreeEx(GetCurrentProcess(), m_Buffer, 0, MEM_RELEASE);
	}
		
	char*	GetBuffPtr() const { return m_Buffer; }
	int		GetBuffSize() const { return m_BufSize; }
};
