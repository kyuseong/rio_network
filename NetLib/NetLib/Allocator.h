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
@brief ���� Fiexed Allocator
������ �������� �޸𸮸� ���� �ӵ��� �Ҵ��ϴ� Allocator ( ��ũ������ �Ҵ� �ϸ� �������� �ʰ� ��� �Ҵ�Ǿ��� )
- Chunk ������ �޸� Ǯ�� �Ҵ��� (alloc_size*chunk_count) ��ŭ
- Chunk���� Chunk����ü�� Linked List�� �����Ǿ���
- Free List �� �����ɶ����� FreeNode�� Linked List�� ���� ������
- Free List �� ������ ���� �̽��� head �� �ֱ��� �޸𸮰� �����ϰ� �ǹǷ� aba problem�� �߻��� ������ ����.( ���� ������ �ذ��ؾ��� ���� )
*/
template<class LOCK>
class FastFixedAllocator
{
	/// @brief Free����Ʈ�� ������ node
	struct FreeNode
	{
		/// @brief ���� ���
		FreeNode* next_;
	};

	/// @brief �޸𸮸� �Ҵ��� ��ũ
	struct Chunk
	{
		/// @brief ���� ��ũ(linked list)
		Chunk* next_;
		/// @brief ���(Chunk)�� ������ ���� buffer�� �ش��ϴ� �κ�(alloc_size*chunk_count)�� �����͸� ��ȯ
		void* get_buffer() { return this + 1; }
	};

	/// @brief ����ȭ ��ü
	LOCK mutex_;
	/// @brief ���� �Ҵ�� �޸� ( ��ũ�� ����Ʈ HEAD )
	Chunk* chunk_;
	/// @brief free node ( ��ũ�� ����Ʈ HEAD )
	FreeNode* free_node_;

public:
	/// @brief �⺻ ��ũ�� �޸� ����
	static const size_t DEFAULT_CHUNK_SIZE = 64;
	/// @brief �Ҵ��� �޸� ������
	size_t ALLOC_SIZE;
	/// @brief ��ũ�� �޸� ����
	size_t CHUNK_SIZE;

	/// @brief ������
	/// @param alloc_size �Ҵ��� �޸��� ������
	/// @param chunk_count �ѹ��� �Ҵ��� �޸��� ����
	FastFixedAllocator(size_t alloc_size, size_t chunk_count = DEFAULT_CHUNK_SIZE)
		: ALLOC_SIZE(alloc_size < sizeof(FreeNode) ? sizeof(FreeNode) : alloc_size),	// �ּ��� FreeNode ��ŭ�� �޸𸮴� �Ҵ�Ǿ�� ���������� �����Ѵ�.(Free Node�� Linked List�� �����ϱ� ������)
		CHUNK_SIZE(chunk_count < 1 ? DEFAULT_CHUNK_SIZE : chunk_count),			// 
		free_node_(NULL),
		chunk_(NULL)
	{
	}

	/// @brief  �Ҹ���
	virtual ~FastFixedAllocator(void)
	{
		free_all();
	}

	/// @brief  alloc size�� �ش��ϴ� �޸𸮸� �Ҵ��Ѵ�.
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

		// free list���� �ϳ� ����
		void* node = free_node_;
		free_node_ = free_node_->next_;
		return node;
	}

	/// @brief  �޸𸮸� ���� �Ѵ�.
	/// @param p ������ �޸�
	void free(void* p)
	{
		if (p != NULL)
		{
			std::lock_guard<LOCK> guard(mutex_);

			// free list �� node�� �־��ش�.
			FreeNode* node = (FreeNode*)p;
			node->next_ = free_node_;
			free_node_ = node;
		}
	}

	/// @brief �Ҵ�� ��� �޸𸮸� �����Ѵ�.(�Ҵ���� �޸𸮸� ����ϸ� �ȵȴ�.)
	void free_all(void)
	{
		std::lock_guard<LOCK> guard(mutex_);

		free_node_ = NULL;
		free_all_chunk(chunk_);
		chunk_ = NULL;
	}

private:

	/// @brief free list�� ä�� �ִ´�.
	bool fill_free_node(bool use_throw)
	{
		// ���� �Ҵ��Ѵ�.
		Chunk* block = alloc_chunk(chunk_, ALLOC_SIZE, CHUNK_SIZE, use_throw);
		if (block == NULL)
			return false;
		// Chunk�� ������ ������ free node �� �����ش�. (linked list)
		// free node �� �������� �����ش�. ( ������� ���Ѽ����� )
		FreeNode* free_node = (FreeNode*)block->get_buffer();
		(char*&)free_node += (ALLOC_SIZE * CHUNK_SIZE) - ALLOC_SIZE;
		for (int i = (int)CHUNK_SIZE - 1; i >= 0; i--, (char*&)free_node -= ALLOC_SIZE)
		{
			free_node->next_ = free_node_;
			free_node_ = free_node;
		}
		return true;
	}

	/// @brief Chunk �� �޸� ������� ������ �°� �Ҵ��Ѵ�.
	/// @param head block�� ù��° Chunk
	/// @param size �޸� ������
	/// @param count �޸��� ����
	Chunk* alloc_chunk(Chunk*& head, size_t size, size_t count, bool use_throw) const
	{
		if (size < 1 || count < 1)
			return NULL;
		Chunk* p = (Chunk*) new char[sizeof(Chunk) + size * count];
		p->next_ = head;
		head = p;
		return p;
	}

	/// @brief ���� ������ �̾��� ��� ���� �����Ѵ�.
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
	/// @brief ������(nocopy)
	FastFixedAllocator(void);
	/// @brief ���������(nocopy)
	FastFixedAllocator(const FastFixedAllocator &) = delete;
	/// @brief �Ҵ���(nocopy)
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
@brief �پ��� �������� �޸𸮸� �Ҵ簡���� Allocator
- FastFixedAllocator�� ����ؼ� ����
- grow_size�� �� ��ŭ ���� �ϴ� alloc_count ������ FastFixedAllocator�� �����ϵ��� �����Ǿ� �ִ�.
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

	/// @brief ������
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

	/// @brief  �Ҹ���
	virtual ~FastGeneralAllocator()
	{
		for (size_t i = 0; i < ALLOC_COUNT; ++i)
		{
			delete fixed_allocator_[i];
		}

		delete[] fixed_allocator_;
	}

	/// @brief �޸𸮸� �Ҵ��ؼ� ��ȯ��
	/// @param nbytes �Ҵ��� �޸� ������
	/// @param use_throw ���� �߻��� bad_alloc�� �߻���ų���� ����
	/// @return void* �޸�
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

	/// @brief �޸𸮸� ������
	/// @param p ������ �޸�
	/// @param nbytes ������ �޸��� ������
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

	/// @brief ũ�⿡ �´� �˸��� Allocator�� ������
	/// @param nbytes ũ��
	/// @return Allocator�� ������
	FastFixedAllocatorType* get_fixed_allocator(size_t nbytes)
	{
		if (nbytes < 1 || nbytes > MAX_ALLOC)
			return 0;
		return fixed_allocator_[(nbytes - 1) / GROW_SIZE];
	}
	/// @brief ���� ������(nocopy)
	FastGeneralAllocator(const FastGeneralAllocator &) = delete;
	/// @brief �Ҵ���(nocopy)
	FastGeneralAllocator & operator = (const FastGeneralAllocator &) = delete;
};

// �޸� �Ҵ����� �޸� ������ ������
constexpr int SMALLOBJALLOCATOR_DEFUALT_GROW_SIZE = 8;

// ��������� ����
constexpr int SMALLOBJALLOCATOR_DEFUALT_ALLOC_COUNT = 100;

// �޸�Ǯ�� �ѹ��� �Ҵ��ϴ� ����
constexpr int SMALLOBJALLOCATOR_DEFUALT_CHUNK_SIZE = 1024;



/**
@brief ���� ������Ʈ�� �Ҵ��Ҷ� ����ϴ� Allocator
- FastGeneralAllocator�� ����ϸ� ���� ����(lock,grow,chunk,alloc)�� �����Ҽ� �ֵ��� �ߴ�.
- Loki�� �ڵ带 �̽���
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
	@brief ������
	*/
	SmallObjAllocator()
	{
	}
};

/**
@brief SmallObj���� ����� Singleton
- �پ��� ������ ���� singleton�� ���Ǿ� ������ ����
- loki���� �̽���
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
	/// @brief ������(nocopy)
	AllocatorSingleton(const AllocatorSingleton &) = delete;
	/// @brief ���� ������(nocopy)
	AllocatorSingleton & operator = (const AllocatorSingleton &) = delete;
};

/**
@brief ���� ������ �޸� �Ҵ��� ���� �ϱ� ���� new/delete ���۷��̼��� ���� Ŭ����
- �� Ŭ������ ��� �޾� ���
- Modern C++ Design �� Loki�� �ڵ带 ���
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

	/// new ������ (throws bad_alloc)
#ifdef _MSC_VER
	/// ms����
	static void * operator new (std::size_t size)

#else

	static void * operator new (std::size_t size) throw (std::bad_alloc)
#endif

	{
		return MyAllocatorSingleton::Instance().alloc(size, true);
	}

	/// new ������ (Non-throwing)
	/// - pool�� �������� ����
	static void * operator new (std::size_t size, const std::nothrow_t &) throw ()
	{
		return std::malloc(size);
	}

	/// new ������ (placement new)
	inline static void * operator new (std::size_t size, void * place)
	{
		return ::operator new(size, place);
	}

	/// delete ������
	static void operator delete (void * p, std::size_t size) throw ()
	{
		MyAllocatorSingleton::Instance().free(p, size);
	}

	/// delete ������( Non-throwing )
	/// - pool�� �������� ����
	static void operator delete (void * p, const std::nothrow_t &) throw()
	{
		std::free(p);
	}

	/// delete ������(placement new)
	inline static void operator delete (void * p, void * place)
	{
		::operator delete (p, place);
	}

protected:
	inline SmallObj(void) {}
private:
	/// @brief ���� ������(nocopy)
//	SmallObj(const SmallObj &) = delete;
	/// @brief �Ҵ���(nocopy)
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
