////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file LinkedList.h
/// \author kkkkkkman
/// \date 2017.05.25
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

///////////////////////////////////////////////////////////////////////////////
/// \brief node
///////////////////////////////////////////////////////////////////////////////
template <class T>
class cLinkedNode
{
private:
	cLinkedNode<T> *m_pPrev;
	cLinkedNode<T> *m_pNext;
	T* m_Data = nullptr;
	cLinkedNode(const cLinkedNode<T>&) = delete;
	cLinkedNode& operator=(const cLinkedNode<T>&) = delete;
public:

	/// 초기화 해준다.
	cLinkedNode() { tie_off(); }
	~cLinkedNode()
	{
		m_pPrev = nullptr;
		m_pNext = nullptr;
		m_Data = nullptr;
	}
	/// 뒤에 추가
	void add_after(cLinkedNode<T> *pLink)
	{
		pLink->m_pPrev = (cLinkedNode<T>*)this;
		pLink->m_pNext = m_pNext;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = (cLinkedNode<T>*)pLink;
	}

	/// 전에 추가
	void add_before(cLinkedNode<T> *pLink)
	{
		pLink->m_pPrev = m_pPrev;
		pLink->m_pNext = (cLinkedNode<T>*)this;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = (cLinkedNode<T>*)pLink;
	}

	/// 지우다
	/// 지우고 끊어줌을 표시한다.
	void remove()
	{
		m_pPrev->m_pNext = m_pNext;
		m_pNext->m_pPrev = m_pPrev;
		tie_off();
	}

	/// 초기화
	void tie_off()
	{
		m_pPrev = m_pNext = (cLinkedNode<T>*)this;
	}

	/// 초기화되었나 확인하기
	bool is_tie_off() const
	{
		return (get_prev() == this) && (get_next() == this);
	}

	/// 다음
	cLinkedNode<T>* get_next() { return m_pNext; }
	/// 이전
	cLinkedNode<T>* get_prev() { return m_pPrev; }
	/// 다음
	void set_next(cLinkedNode<T>* pNext) { m_pNext = pNext; }
	/// 이전
	void set_prev(cLinkedNode<T>* pPrev) { m_pPrev = pPrev; }
	/// 다음
	const cLinkedNode<T>* get_next() const { return m_pNext; }
	/// 이전
	const cLinkedNode<T>* get_prev() const { return m_pPrev; }
	/// point 구하기
	T* get_data() { return reinterpret_cast<T*>(m_Data); }
	const T* get_data() const { return reinterpret_cast<const T*>(m_Data); }
	void set_data(T* Data) { m_Data = Data; }
};


///////////////////////////////////////////////////////////////////////////////
/// \brief cLinkedList 더블링크드 리스트
///////////////////////////////////////////////////////////////////////////////
template <class T>
class cLinkedList
{
public:
	class iterator
	{
	public:
		typedef iterator my_t;
		typedef cLinkedNode<T>* ptr_type;
		typedef T value_type;
		typedef T& reference;
		typedef T* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		iterator(ptr_type ptr) : ptr_(ptr) { }
		my_t operator++() { my_t i = *this; ptr_ = ptr_->get_next(); return i; }
		my_t operator++(int junk) { ptr_= ptr_->get_next(); return *this; }
		pointer operator*() { return ptr_->get_data(); }
		pointer operator->() { return ptr_->get_data(); }
		bool operator==(const my_t& rhs) { return ptr_ == rhs.ptr_; }
		bool operator!=(const my_t& rhs) { return ptr_ != rhs.ptr_; }
		ptr_type get_ptr() { return ptr_; }
		
	private:
		ptr_type ptr_;
	};
	
	iterator begin() { return iterator(m_Head.get_next()); }
	iterator end() { return iterator((cLinkedNode<T>*)&m_Head); }

	class const_iterator
	{
	public:
		typedef const_iterator my_t;
		typedef const cLinkedNode<T>* ptr_type;
		typedef const T value_type;
		typedef const T& reference;
		typedef const T* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		const_iterator(ptr_type ptr) : ptr_(ptr) { }
		my_t operator++() { my_t i = *this; ptr_ = ptr_->get_next(); return i; }
		my_t operator++(int junk) { ptr_ = ptr_->get_next(); return *this; }
		pointer operator*() { return ptr_->get_data(); }
		pointer operator->() { return ptr_->get_data(); }
		bool operator==(const my_t& rhs) { return ptr_ == rhs.ptr_; }
		bool operator!=(const my_t& rhs) { return ptr_ != rhs.ptr_; }
	private:
		ptr_type ptr_;
	};

	const_iterator begin() const{ return const_iterator(m_Head.get_next()); }
	const_iterator end() const{ return const_iterator((cLinkedNode<T>*)&m_Head); }

	cLinkedList() {}
	~cLinkedList() {}

	/// 다 지움
	void	clear()
	{
		while (!empty())
		{
			auto top = begin();
			top.get_ptr()->remove();
		}

		m_Head.remove();
	}

	/// 비어 있는지 확인하기
	bool	empty() const { return m_Head.is_tie_off(); }

	/// 갯수를 센다.
	size_t	size() const
	{
		size_t nResult = 0;
		for (const_iterator it = begin(); it != end(); ++it)
			nResult++;

		return nResult;
	}

	/// After 뒤에 Node 추가
	void	insert_after(cLinkedNode<T> *pAfter, cLinkedNode<T> *pLink)
	{
		ASSERT(is_in_list(pAfter), L"cLinkedList - add_after - Error: 링크에 포함된 값이 아님");
		ASSERT(!is_in_list(pLink), L"cLinkedList - add_after - Error: 링크에 이미 포함된 값임");

		pAfter->add_after(pLink);
		// m_Count++;
	}

	/// Before 앞에 Node 추가
	void	insert_before(cLinkedNode<T> *pBefore, cLinkedNode<T> *pLink)
	{
		ASSERT(is_in_list(pBefore), L"cLinkedList - add_before - Error: 링크에 포함된 값이 아님");
		ASSERT(!is_in_list(pLink), L"cLinkedList - add_before - Error: 링크에 이미 포함된 값임");

		pBefore->add_before(pLink);
		// m_Count++;
	}

	/// 헤드에서 추가
	void	push_front(cLinkedNode<T> *pLink)
	{
		ASSERT(!is_in_list(pLink), L"cLinkedList - AddHead - Error: 링크에 이미 포함된 값임");
		m_Head.add_after(pLink);
		// m_Count++;
	}

	/// 뒤에서 추가
	void	push_back(cLinkedNode<T> *pLink)
	{
		ASSERT(!is_in_list(pLink), L"cLinkedList - AddTail - Error: 링크에 이미 포함된 값임");
		m_Head.add_before(pLink);
		// m_Count++;
	}

	/// 삭제
	void	erase(cLinkedNode<T>* pLink)
	{
		ASSERT(pLink != &m_Head, L"cLinkedList - remove - Error: Head를 지우진 말자");
		ASSERT(is_in_list(pLink), L"cLinkedList - remove - Error: 들어 있는게 아니다");
		pLink->remove();
		// m_Count--;
	}

	iterator erase(iterator iter)
	{	
		ASSERT(iter.get_ptr() != &m_Head, L"cLinkedList - remove - Error: Head를 지우진 말자");
		ASSERT(is_in_list(iter.get_ptr()), L"cLinkedList - remove - Error: 들어 있는게 아니다");

		iterator next (iter.get_ptr()->get_next());
		erase(iter.get_ptr());
		// m_Count--;
		return next;
	}
	
	/// 찾기
	iterator find(const T* Data)
	{
		for (auto it = begin(); it != end(); ++it)
		{
			if (it->get_data() == Data)
				return it;
		}
		return end();
	}

	/// 리스트 안에 들어있는거야?
	bool is_in_list(cLinkedNode<T>* pLink) const
	{
		for (auto it = begin(); it != end(); ++it)
		{
			if (*it == pLink->get_data())
				return true;
		}
		return false;
	}
	
private:
	cLinkedList(const cLinkedList&) = delete;
	cLinkedList& operator=(const cLinkedList&) = delete;

	/// 헤드
	cLinkedNode<T>	m_Head;

	// int m_Count = 0;
};

