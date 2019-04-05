#pragma once

#include <mutex>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <queue>

#include "Mutex.h"

template <class TYPE>
class SingletonT
{
public:
	/// Global access point to the Singleton.
	static TYPE& Instance(void)
	{

		static TYPE instance_;
		return instance_;
	}
};


/**
@brief 빠른 Fiexed Allocator
고정된 사이즈의 메모리를 빠른 속도로 할당하는 Allocator ( 정크단위로 할당 하며 지워지지 않고 계속 할당되어짐 )
- Chunk 단위로 메모리 풀을 할당함 (alloc_size*chunk_count) 만큼
- Chunk들은 Chunk구조체로 Linked List로 관리되어짐
- Free List 에 관리될때에는 FreeNode로 Linked List로 만들어서 관리함
- Free List 의 관리시 성능 이슈로 head 에 최근의 메모리가 존재하게 되므로 aba problem이 발생할 소지가 많다.( 차후 버젼에 해결해야할 문제 )
*/
template<class LOCK>
class FastFixedAllocator
{
	/// @brief Free리스트를 관리할 node
	struct FreeNode
	{
		/// @brief 다음 노드
		FreeNode* next_;
	};

	/// @brief 메모리를 할당할 정크
	struct Chunk
	{
		/// @brief 다음 정크(linked list)
		Chunk* next_;
		/// @brief 헤더(Chunk)를 제외한 실제 buffer에 해당하는 부분(alloc_size*chunk_count)의 포인터를 반환
		void* get_buffer() { return this + 1; }
	};

	/// @brief 동기화 객체
	LOCK mutex_;
	/// @brief 실제 할당된 메모리 ( 링크드 리스트 HEAD )
	Chunk* chunk_;
	/// @brief free node ( 링크드 리스트 HEAD )
	FreeNode* free_node_;

public:
	/// @brief 기본 정크의 메모리 갯수
	static const size_t DEFAULT_CHUNK_SIZE = 64;
	/// @brief 할당할 메모리 사이즈
	size_t ALLOC_SIZE;
	/// @brief 정크의 메모리 갯수
	size_t CHUNK_SIZE;

	/// @brief 생성자
	/// @param alloc_size 할당할 메모리의 사이즈
	/// @param chunk_count 한번에 할당할 메모리의 갯수
	FastFixedAllocator(size_t alloc_size, size_t chunk_count = DEFAULT_CHUNK_SIZE)
		: ALLOC_SIZE(alloc_size < sizeof(FreeNode) ? sizeof(FreeNode) : alloc_size),	// 최소한 FreeNode 만큼의 메모리는 할당되어야 정상적으로 동작한다.(Free Node를 Linked List로 관리하기 때문에)
		CHUNK_SIZE(chunk_count < 1 ? DEFAULT_CHUNK_SIZE : chunk_count),			// 
		free_node_(NULL),
		chunk_(NULL)
	{
	}

	/// @brief  소멸자
	virtual ~FastFixedAllocator(void)
	{
		free_all();
	}

	/// @brief  alloc size에 해당하는 메모리를 할당한다.
	void* alloc(size_t nbytes = 0, bool use_throw = false)
	{
		if (nbytes > ALLOC_SIZE)
			return 0;

		std::lock_guard<LOCK> guard(mutex_);

		if (free_node_ == NULL)
		{
			if (!fill_free_node(use_throw))
				return NULL;
		}

		// free list에서 하나 꺼냄
		void* node = free_node_;
		free_node_ = free_node_->next_;
		return node;
	}

	/// @brief  메모리를 해제 한다.
	/// @param p 해제할 메모리
	void free(void* p)
	{
		if (p != NULL)
		{
			std::lock_guard<LOCK> guard(mutex_);

			// free list 에 node로 넣어준다.
			FreeNode* node = (FreeNode*)p;
			node->next_ = free_node_;
			free_node_ = node;
		}
	}

	/// @brief 할당된 모든 메모리를 해제한다.(할당받은 메모리를 사용하면 안된다.)
	void free_all(void)
	{
		std::lock_guard<LOCK> guard(mutex_);

		free_node_ = NULL;
		free_all_chunk(chunk_);
		chunk_ = NULL;
	}

private:

	/// @brief free list을 채워 넣는다.
	bool fill_free_node(bool use_throw)
	{
		// 블럭을 할당한다.
		Chunk* block = alloc_chunk(chunk_, ALLOC_SIZE, CHUNK_SIZE, use_throw);
		if (block == NULL)
			return false;
		// Chunk의 데이터 영역을 free node 로 엮어준다. (linked list)
		// free node 를 역순으로 역어준다. ( 디버깅이 편한순서로 )
		FreeNode* free_node = (FreeNode*)block->get_buffer();
		(char*&)free_node += (ALLOC_SIZE * CHUNK_SIZE) - ALLOC_SIZE;
		for (int i = (int)CHUNK_SIZE - 1; i >= 0; i--, (char*&)free_node -= ALLOC_SIZE)
		{
			free_node->next_ = free_node_;
			free_node_ = free_node;
		}
		return true;
	}

	/// @brief Chunk 을 메모리 사이즈와 갯수에 맞게 할당한다.
	/// @param head block의 첫번째 Chunk
	/// @param size 메모리 사이즈
	/// @param count 메모리의 갯수
	Chunk* alloc_chunk(Chunk*& head, size_t size, size_t count, bool use_throw) const
	{
		if (size < 1 || count < 1)
			return NULL;
		Chunk* p = (Chunk*) new char[sizeof(Chunk) + size * count];
		p->next_ = head;
		head = p;
		return p;
	}

	/// @brief 현재 블럭에서 이어진 모든 블럭을 해제한다.
	void free_all_chunk(Chunk* p) const
	{
		while (p != NULL)
		{
			char* bytes = (char*)p;
			Chunk* next = p->next_;
			delete[] bytes;
			p = next;
		}
	}

private:
	/// @brief 생성자(nocopy)
	FastFixedAllocator(void);
	/// @brief 복사생성자(nocopy)
	FastFixedAllocator(const FastFixedAllocator &) = delete;
	/// @brief 할당자(nocopy)
	FastFixedAllocator & operator = (const FastFixedAllocator &) = delete;
};

inline void* default_allocator(size_t nbytes, bool use_throw)
{
	void * p = ::std::malloc(nbytes);
	if (use_throw && (NULL == p))
		throw std::bad_alloc();
	return p;
}

inline void default_deallocator(void * p)
{
	::std::free(p);
}

/**
@brief 다양한 사이즈의 메모리를 할당가능한 Allocator
- FastFixedAllocator를 사용해서 구현
- grow_size의 양 만큼 증가 하는 alloc_count 갯수의 FastFixedAllocator이 존재하도록 구현되어 있다.
*/
template<class LOCK,
	std::size_t grow_size,
	std::size_t chunk,
	std::size_t alloc_count
>
class FastGeneralAllocator
{
	typedef FastFixedAllocator<LOCK> FastFixedAllocatorType;
	FastFixedAllocatorType** fixed_allocator_;
public:
	const size_t GROW_SIZE = grow_size;
	const size_t CHUNK = chunk;
	const size_t ALLOC_COUNT = alloc_count;
	const size_t MAX_ALLOC = grow_size * alloc_count;

	/// @brief 생성자
	FastGeneralAllocator()
	{
		size_t size = 0;
		fixed_allocator_ = new FastFixedAllocatorType*[ALLOC_COUNT];
		for (size_t i = 0; i < ALLOC_COUNT; ++i)
		{
			size += GROW_SIZE;
			fixed_allocator_[i] = new FastFixedAllocatorType(size, chunk);
		}
	}

	/// @brief  소멸자
	virtual ~FastGeneralAllocator()
	{
		for (size_t i = 0; i < ALLOC_COUNT; ++i)
		{
			delete fixed_allocator_[i];
		}

		delete[] fixed_allocator_;
	}

	/// @brief 메모리를 할당해서 반환함
	/// @param nbytes 할당할 메모리 사이즈
	/// @param use_throw 예외 발생시 bad_alloc를 발생시킬껀지 유무
	/// @return void* 메모리
	void *alloc(size_t nbytes = 0, bool use_throw = false)
	{
		if (nbytes > MAX_ALLOC)
			return default_allocator(nbytes, use_throw);
		FastFixedAllocatorType* allocator = get_fixed_allocator(nbytes);
		if (allocator == 0)
		{
			if (use_throw)
				throw std::bad_alloc();
			return 0;
		}

		return allocator->alloc(nbytes);
	}

	/// @brief 메모리를 해제함
	/// @param p 해제할 메모리
	/// @param nbytes 해제할 메모리의 사이즈
	void free(void* p, size_t nbytes)
	{
		if (nbytes > MAX_ALLOC)
		{
			default_deallocator(p);
			return;
		}
		FastFixedAllocatorType* allocator = get_fixed_allocator(nbytes);
		if (allocator == 0)
			return;

		allocator->free(p);
	}

	/// @brief 크기에 맞는 알맞은 Allocator의 포인터
	/// @param nbytes 크기
	/// @return Allocator의 포인터
	FastFixedAllocatorType* get_fixed_allocator(size_t nbytes)
	{
		if (nbytes < 1 || nbytes > MAX_ALLOC)
			return 0;
		return fixed_allocator_[(nbytes - 1) / GROW_SIZE];
	}
	/// @brief 복사 생성자(nocopy)
	FastGeneralAllocator(const FastGeneralAllocator &) = delete;
	/// @brief 할당자(nocopy)
	FastGeneralAllocator & operator = (const FastGeneralAllocator &) = delete;
};

// 메모리 할당자의 메모리 사이즈 증가량
constexpr int SMALLOBJALLOCATOR_DEFUALT_GROW_SIZE = 8;

// 얼로케이터 갯수
constexpr int SMALLOBJALLOCATOR_DEFUALT_ALLOC_COUNT = 100;

// 메모리풀을 한번에 할당하는 갯수
constexpr int SMALLOBJALLOCATOR_DEFUALT_CHUNK_SIZE = 1024;



/**
@brief 작은 오브젝트를 할당할때 사용하는 Allocator
- FastGeneralAllocator를 사용하며 여러 전략(lock,grow,chunk,alloc)을 구사할수 있도록 했다.
- Loki의 코드를 이식함
*/
template
<
	class LOCK,
	std::size_t grow_size = SMALLOBJALLOCATOR_DEFUALT_GROW_SIZE,
	std::size_t chunk = SMALLOBJALLOCATOR_DEFUALT_CHUNK_SIZE,
	std::size_t alloc_count = SMALLOBJALLOCATOR_DEFUALT_ALLOC_COUNT
>
class SmallObjAllocator : public FastGeneralAllocator<LOCK, grow_size, chunk, alloc_count>
{
public:
	/**
	@brief 생성자
	*/
	SmallObjAllocator()
	{
	}
};

/**
@brief SmallObj에서 사용할 Singleton
- 다양한 전략에 따라 singleton이 사용되어 지도록 했음
- loki에서 이식함
*/
template
<
	class LOCK,
	std::size_t grow_size = SMALLOBJALLOCATOR_DEFUALT_GROW_SIZE,
	std::size_t chunk = SMALLOBJALLOCATOR_DEFUALT_CHUNK_SIZE,
	std::size_t alloc_count = SMALLOBJALLOCATOR_DEFUALT_ALLOC_COUNT
>
class AllocatorSingleton : public SmallObjAllocator<LOCK, grow_size, chunk, alloc_count>
{
public:
	typedef AllocatorSingleton< LOCK, grow_size, chunk, alloc_count > MyAllocator;
	typedef SingletonT< MyAllocator> MyAllocatorSingleton;
	inline static AllocatorSingleton & Instance(void)
	{
		return MyAllocatorSingleton::Instance();
	}
	AllocatorSingleton() = default;
	/// @brief 생성자(nocopy)
	AllocatorSingleton(const AllocatorSingleton &) = delete;
	/// @brief 복사 생성자(nocopy)
	AllocatorSingleton & operator = (const AllocatorSingleton &) = delete;
};

/**
@brief 작은 단위의 메모리 할당을 방지 하기 위한 new/delete 오퍼레이션을 가진 클래스
- 이 클래스를 상속 받아 사용
- Modern C++ Design 과 Loki의 코드를 사용
@code
class Test : public SmallObj<>
{
char a[10];
};
class Test1 : public Test
{
char a[10];
};

class TestSingle : public SmallObj<ACE_Null_Mutex>
{
char a[10];
};

...

Test* t = new Test;
delete  t;
Test1* t1 = new Test1;
delete  t1;
TestSingle* t2 = new TestSingle;
delete  t2;

@endcode
*/

template
<

	std::size_t grow_size = SMALLOBJALLOCATOR_DEFUALT_GROW_SIZE,
	std::size_t chunk = SMALLOBJALLOCATOR_DEFUALT_CHUNK_SIZE,
	std::size_t alloc_count = SMALLOBJALLOCATOR_DEFUALT_ALLOC_COUNT,
	class LOCK = std::mutex
>
class SmallObj
{
public:
	typedef AllocatorSingleton<LOCK, grow_size, chunk, alloc_count > ObjAllocatorSingleton;
	typedef typename ObjAllocatorSingleton::MyAllocatorSingleton MyAllocatorSingleton;

	/// new 연산자 (throws bad_alloc)
#ifdef _MSC_VER
	/// ms버젼
	static void * operator new (std::size_t size)

#else

	static void * operator new (std::size_t size) throw (std::bad_alloc)
#endif

	{
		return MyAllocatorSingleton::Instance().alloc(size, true);
	}

	/// new 연산자 (Non-throwing)
	/// - pool를 제공하지 않음
	static void * operator new (std::size_t size, const std::nothrow_t &) throw ()
	{
		return std::malloc(size);
	}

	/// new 연산자 (placement new)
	inline static void * operator new (std::size_t size, void * place)
	{
		return ::operator new(size, place);
	}

	/// delete 연산자
	static void operator delete (void * p, std::size_t size) throw ()
	{
		MyAllocatorSingleton::Instance().free(p, size);
	}

	/// delete 연산자( Non-throwing )
	/// - pool를 제공하지 않음
	static void operator delete (void * p, const std::nothrow_t &) throw()
	{
		std::free(p);
	}

	/// delete 연산자(placement new)
	inline static void operator delete (void * p, void * place)
	{
		::operator delete (p, place);
	}

protected:
	inline SmallObj(void) {}
private:
	/// @brief 복사 생성자(nocopy)
//	SmallObj(const SmallObj &) = delete;
	/// @brief 할당자(nocopy)
//	SmallObj & operator = (const SmallObj &) = delete;

};


///////////////////////////////////////////////////////////////////////////
//	class SmallObjectAllocPolicy
///////////////////////////////////////////////////////////////////////////

template<typename LOCK, typename T>
class SmallObjectAllocPolicy {
public:
	//	typedefs
	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
public:
	template<typename U>
	struct rebind {
		typedef SmallObjectAllocPolicy<LOCK, U> other;
	};

public:


	T * address(T& _Val) const noexcept
	{
		return (_STD addressof(_Val));
	}

	const T * address(const T& _Val) const noexcept
	{
		return (_STD addressof(_Val));
	}

	constexpr SmallObjectAllocPolicy() noexcept
	{
	}

	template <typename LOCK2, typename T2>
	constexpr SmallObjectAllocPolicy(SmallObjectAllocPolicy<LOCK2, T2> const&) noexcept {}

	void deallocate(pointer _Ptr, size_type _Count)
	{
		AllocatorSingleton<LOCK>::Instance().free(_Ptr, sizeof(T) * _Count);
	}

	_DECLSPEC_ALLOCATOR pointer allocate(size_type _Count)
	{
		return (pointer)AllocatorSingleton<LOCK>::Instance().alloc(sizeof(T)* _Count);
	}

	_DECLSPEC_ALLOCATOR pointer allocate(size_type _Count, const void *)
	{
		return (allocate(_Count));
	}


	template<class _Objty,
		class... _Types>
		void construct(_Objty * const _Ptr, _Types&&... _Args)
	{	// construct _Objty(_Types...) at _Ptr
		::new (const_cast<void *>(static_cast<const volatile void *>(_Ptr)))
			_Objty(_STD forward<_Types>(_Args)...);
	}

	template<class _Uty>
	void destroy(_Uty * const _Ptr)
	{	// destroy object at _Ptr
		_Ptr->~_Uty();
	}

#ifdef max
#undef max
	//	size
	inline size_type max_size() const { return std::numeric_limits<size_type>::max() / sizeof(T); }
#endif

private:
};

template<typename L, typename T>
inline bool operator==(SmallObjectAllocPolicy< L, T> const&, SmallObjectAllocPolicy< L, T> const&) {
	return true;
}
template<typename L, typename T>
inline bool operator!=(SmallObjectAllocPolicy< L, T> const&, SmallObjectAllocPolicy< L, T> const&) {
	return false;
}

template<typename L, typename T, typename L2, typename T2>
inline bool operator==(SmallObjectAllocPolicy<L, T> const&, SmallObjectAllocPolicy<L2, T2> const&) {
	return false;
}

template<typename L, typename T, typename L2, typename T2>
inline bool operator!=(SmallObjectAllocPolicy<L, T> const&, SmallObjectAllocPolicy<L2, T2> const&) {
	return false;
}

template<typename L, typename T, typename L2, typename OtherAllocator>
inline bool operator==(SmallObjectAllocPolicy<L, T> const&, OtherAllocator const&) {
	return false;
}

template<class T>
using t_vector = std::vector<T, SmallObjectAllocPolicy<SpinLock, T>>;

template<class _Kty,
	class _Pr = std::less<_Kty>,
	class _Alloc = SmallObjectAllocPolicy<SpinLock, _Kty>>
	using t_set = std::set<_Kty, _Pr, _Alloc>;

template<class _Kty,
	class _Ty,
	class _Pr = std::less<_Kty>,
	class _Alloc = SmallObjectAllocPolicy<SpinLock, std::pair<const _Kty, _Ty>>>
	using t_map = std::map<_Kty, _Ty, _Pr, _Alloc>;

template<class _Kty,
	class _Ty,
	class _Hasher = std::hash<_Kty>,
	class _Keyeq = std::equal_to<_Kty>,
	class _Alloc = SmallObjectAllocPolicy<SpinLock, std::pair<const _Kty, _Ty>>>
	using t_unordered_map = std::unordered_map<_Kty, _Ty, _Hasher, _Keyeq, _Alloc>;
