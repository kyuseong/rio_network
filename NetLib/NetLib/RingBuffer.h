#pragma once

// �� �۹�
// 
// head tail�� ��� �������� ��ġ�� 
// full�϶� head �� tail�� ��ġ�� ����
// ���� ���� ���۸� ���� ������
// |-------====================------|
//        |                   |
//       Head                Tail     

class RingBuffer final
{
private:
	char*	m_pBuffer;		// �Ҵ�� ����
	int		m_iBufSize;		// ���� �Ҵ�� ���� ������

	int		m_iHeadPos;		// ������ �ִ� ������
	int		m_iTailPos;		// �� ������

public:
	RingBuffer(int iSize = 4096);
	~RingBuffer();

	// ����
	void	Clear() { m_iHeadPos = 0; m_iTailPos = 0; }
	// �����͸� �ִ´�.
	bool	PutData(char *Data, int iLen);
	// �����͸� �����ʰ� �Ҵ縸 ���ش�.
	bool	PutEmptyData(int iLen);
	// �����͸� ������.
	bool	GetData(char *Data, int iLen);
	// �����͸� �����.
	bool	PopData(int Len);
	// ��� �ִ� ������
	int		GetDataLength() const;
	// ������ �ִ� ������
	int		GetCapacitySize() const { return GetBuffSize() - GetDataLength() - 1; }
	// ���� ����Ʈ
	char*	GetBuffPtr() const { return m_pBuffer; }
	// ���� ������
	int		GetBuffSize() const { return m_iBufSize; }
	// ����Ʈ ����
	char*	GetDataPtr()	const { return m_pBuffer + m_iHeadPos; }
	// ���� ��ġ
	int		GetTailPos() const { return m_iTailPos; }
	// ��á��?
	bool	IsFull() const { return GetDataLength() == GetBuffSize() - 1; }
	// �����͸� ������ �ֳ�?
	bool	CanPutData(const int Len) const { return (Len < m_iBufSize - GetDataLength()); }
	// �и��Ǿ� �ֳ�? 
	//  Second                     First
	// |=======--------------------======|
	//        |                    |
	//       Tail                Head     
	bool	CheckSplit(const int Len) const { return (Len + m_iTailPos >= m_iBufSize); }
	bool	IsSplit() const { return m_iHeadPos > m_iTailPos; }
	// �и��� ù��° ������ ����Ʈ�� ���Ѵ�.
	// BeforeIndex ��ŭ ���� �������� ������ ����Ʈ�� ���Ѵ�.
	char*	GetFirstDataPtr(int BeforePos) const {
		if (BeforePos > m_iTailPos) {
			BeforePos -= m_iTailPos;
			return m_pBuffer + m_iBufSize - BeforePos;
		}
		return m_pBuffer + m_iTailPos - BeforePos;
	}
	// �и��� �ι�° ������ ����Ʈ�� ���Ѵ�.
	// BeforeIndex ��ŭ ���� �������� ������ ����Ʈ�� ���Ѵ�.
	char*	GetSecondDataPtr(int BeforePos) const { return BeforePos > m_iTailPos ? m_pBuffer : nullptr; }

	// �и� �Ǿ��� ��� ù��° ������
	char* GetFirstData() { return m_pBuffer + m_iHeadPos; }
	int GetFirstDataOffset() { return m_iHeadPos; }
	// �и� �Ǿ��� ��� ù��° ������ ����
	int GetFirstDataLen() const { return IsSplit() ? m_iBufSize - m_iHeadPos : m_iTailPos - m_iHeadPos; }
	// �и� �Ǿ��� ��� �ι�° ������
	char* GetSecondData() { return IsSplit() ? m_pBuffer : nullptr; }
	// �и� �Ǿ��� ��� �ι�° ������
	int GetSecondDataLen() const { return IsSplit() ? m_iTailPos : 0; }

private:
	void GlowBuffer();

};


inline
RingBuffer::RingBuffer(int iSize) : m_iBufSize(iSize), m_iHeadPos(0), m_iTailPos(0)
{
	m_pBuffer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, m_iBufSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
}

inline
RingBuffer::~RingBuffer()
{
	VirtualFreeEx(GetCurrentProcess(), m_pBuffer, 0, MEM_RELEASE);

}

inline
int RingBuffer::GetDataLength() const
{
	int iLen = m_iTailPos - m_iHeadPos;
	if (iLen < 0) iLen = m_iBufSize + iLen;
	return iLen;
}

inline
void RingBuffer::GlowBuffer()
{
	int iPreBufSize = m_iBufSize;
	m_iBufSize <<= 1;

	char* pNewData = new char[m_iBufSize];
	memcpy(pNewData, m_pBuffer, iPreBufSize);
	if (m_iTailPos < m_iHeadPos) {
		memcpy(pNewData + iPreBufSize, m_pBuffer, m_iTailPos);
		m_iTailPos += iPreBufSize;
	}
	delete[] m_pBuffer;
	m_pBuffer = pNewData;
}

inline
bool	RingBuffer::PutEmptyData(int iLen)
{
	if (iLen < 0)
		return false;
	if (CanPutData(iLen) == false)
		return false;
	if (CheckSplit(iLen)) {
		int iFirstLen = m_iBufSize - m_iTailPos;
		int iSecondLen = iLen - iFirstLen;

		if (iSecondLen) {
			m_iTailPos = iSecondLen;
		}
		else
			m_iTailPos = 0;
	}
	else {
		m_iTailPos += iLen;
	}
	return true;
}

inline
bool RingBuffer::PutData(char * Data, int iLen)
{
	if (iLen < 0)
		return false;
	if (CanPutData(iLen) == false)
		return false;
	if (CheckSplit(iLen)) {
		int iFirstLen = m_iBufSize - m_iTailPos;
		int iSecondLen = iLen - iFirstLen;

		memcpy(m_pBuffer + m_iTailPos, Data, iFirstLen);
		if (iSecondLen) {
			memcpy(m_pBuffer, Data + iFirstLen, iSecondLen);
			m_iTailPos = iSecondLen;
		}
		else
			m_iTailPos = 0;
	}
	else {
		memcpy(m_pBuffer + m_iTailPos, Data, iLen);
		m_iTailPos += iLen;
	}
	return true;
}


inline
bool RingBuffer::GetData(char * Data, int Len)
{
	if (Len > GetDataLength())
		return false;

	if (Len < m_iBufSize - m_iHeadPos) {
		memcpy(Data, m_pBuffer + m_iHeadPos, Len);
	}
	else {
		int iFirstLen, iSecondLen;
		iFirstLen = m_iBufSize - m_iHeadPos;
		iSecondLen = Len - iFirstLen;

		memcpy(Data, m_pBuffer + m_iHeadPos, iFirstLen);
		if (iSecondLen) {
			memcpy(Data + iFirstLen, m_pBuffer, iSecondLen);
		}
	}

	return true;
}

inline
bool RingBuffer::PopData(int Len)
{
	m_iHeadPos += Len;
	m_iHeadPos %= m_iBufSize;
	return m_iHeadPos != m_iTailPos;
}

