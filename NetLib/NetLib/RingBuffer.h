#pragma once

// 링 퍼버
// 
// head tail은 비어 있을때만 겹치고 
// full일때 head 와 tail이 겹치지 않음
// 고정 길이 버퍼를 가진 링버퍼
// |-------====================------|
//        |                   |
//       Head                Tail     

class RingBuffer final
{
private:
	char*	m_pBuffer;		// 할당된 버퍼
	int		m_iBufSize;		// 실제 할당된 버프 사이즈

	int		m_iHeadPos;		// 읽을수 있는 포지션
	int		m_iTailPos;		// 쓴 포지션

public:
	RingBuffer(int iSize = 4096);
	~RingBuffer();

	// 리셋
	void	Clear() { m_iHeadPos = 0; m_iTailPos = 0; }
	// 데이터를 넣는다.
	bool	PutData(char *Data, int iLen);
	// 데이터를 넣지않고 할당만 해준다.
	bool	PutEmptyData(int iLen);
	// 데이터를 꺼낸다.
	bool	GetData(char *Data, int iLen);
	// 데이터를 지운다.
	bool	PopData(int Len);
	// 들어 있는 데이터
	int		GetDataLength() const;
	// 넣을수 있는 사이즈
	int		GetCapacitySize() const { return GetBuffSize() - GetDataLength() - 1; }
	// 버퍼 포인트
	char*	GetBuffPtr() const { return m_pBuffer; }
	// 버퍼 사이즈
	int		GetBuffSize() const { return m_iBufSize; }
	// 데이트 버퍼
	char*	GetDataPtr()	const { return m_pBuffer + m_iHeadPos; }
	// 꼬리 위치
	int		GetTailPos() const { return m_iTailPos; }
	// 꽉찼어?
	bool	IsFull() const { return GetDataLength() == GetBuffSize() - 1; }
	// 데이터를 넣을수 있냐?
	bool	CanPutData(const int Len) const { return (Len < m_iBufSize - GetDataLength()); }
	// 분리되어 있냐? 
	//  Second                     First
	// |=======--------------------======|
	//        |                    |
	//       Tail                Head     
	bool	CheckSplit(const int Len) const { return (Len + m_iTailPos >= m_iBufSize); }
	bool	IsSplit() const { return m_iHeadPos > m_iTailPos; }
	// 분리된 첫번째 데이터 포인트를 구한다.
	// BeforeIndex 만큼 이전 데이터의 데이터 포인트를 구한다.
	char*	GetFirstDataPtr(int BeforePos) const {
		if (BeforePos > m_iTailPos) {
			BeforePos -= m_iTailPos;
			return m_pBuffer + m_iBufSize - BeforePos;
		}
		return m_pBuffer + m_iTailPos - BeforePos;
	}
	// 분리된 두번째 데이터 포인트를 구한다.
	// BeforeIndex 만큼 이전 데이터의 데이터 포인트를 구한다.
	char*	GetSecondDataPtr(int BeforePos) const { return BeforePos > m_iTailPos ? m_pBuffer : nullptr; }

	// 분리 되었을 경우 첫번째 데이터
	char* GetFirstData() { return m_pBuffer + m_iHeadPos; }
	int GetFirstDataOffset() { return m_iHeadPos; }
	// 분리 되었을 경우 첫번째 데이터 길이
	int GetFirstDataLen() const { return IsSplit() ? m_iBufSize - m_iHeadPos : m_iTailPos - m_iHeadPos; }
	// 분리 되었을 경우 두번째 데이터
	char* GetSecondData() { return IsSplit() ? m_pBuffer : nullptr; }
	// 분리 되었을 경우 두번째 데이터
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

