#pragma once

#ifndef ASSERT
#define ASSERT(a,b)
#endif

#include "Allocator.h"
#include "LinkedList.h"

class BaseObjectPool {
public:
	virtual ~BaseObjectPool() {}

	virtual void *AllocVoid() = 0;
	virtual void FreeVoid(void *ptr) = 0;
};

// 간단한 Object Pool
template<class T, class LOCK = NullMutex>
class ObjectPool : public BaseObjectPool
{
	FastFixedAllocator< LOCK > m_Alloc;
	
public:
	ObjectPool(size_t chunk_count = 32) : m_Alloc(sizeof(T), chunk_count)
	{
	}

	virtual ~ObjectPool() {}

	// 메모리 할당
	template<class... _Types>
	T * Allocate(_Types&&... _Args)
	{

		T *Ret = nullptr;

		Ret = (T *)m_Alloc.alloc();

		if (Ret)
		{
			::new (Ret) T(std::forward<_Types>(_Args)...);

			//::new (Ret) T;
		}

		return Ret;
	}
	// 해제
	void Free(T *Obj)
	{
		Obj->~T();

		m_Alloc.free(Obj);
	}

	virtual void *AllocVoid() {
		return Allocate();
	}

	virtual void FreeVoid(void *ptr) {
		Free((T*)ptr);
	}
};

// 재사용하는 오브젝트 풀
template<class T, class LOCK = NullMutex>
class ReusableObjectPool : public BaseObjectPool
{
	// 실제로 메모리 풀
	ObjectPool<T, NullMutex> objpool;
	// Node Pool (free_node 정보 저장할)
	ObjectPool<cLinkedNode<T>, NullMutex> node_pool;
	// free_list
	cLinkedList<T> free_list;
	// lock
	LOCK mutex;
public:

	ReusableObjectPool(size_t chunk_count = 32) : objpool(chunk_count), node_pool(chunk_count)
	{
	}

	virtual ~ReusableObjectPool()
	{
		std::lock_guard<LOCK> g(mutex);

		// free_list에 남아 있으면
		while (false == free_list.empty())
		{
			// top을 구해서
			auto top = free_list.begin();
			// node 정보를 구하고
			auto Node = top.get_ptr();
			// obj 를 삭제해주고
			objpool.Free(top.get_ptr()->get_data());
			// free_list에서 제거하고
			free_list.erase(top);
			// Node을 풀에 다시 넣어준다.
			node_pool.Free(Node);
		}
	}

	template<class... _Types>
	T * Allocate(_Types&&... _Args)
	{
		std::lock_guard<LOCK> g(mutex);
		// free_list 에 있냐?
		if (free_list.empty())
		{
			// 없으면 그냥 할당해서 줌
			T* obj = objpool.Allocate(std::forward<_Types>(_Args)...);

			return obj;
		}

		// free_list에 있으면 처음꺼 꺼내서 준다.
		auto Node = free_list.begin();
		// obj을 구해온다.
		T*  obj = Node.get_ptr()->get_data();
		// null로 셋팅해주고(혹시 모르니까)
		Node.get_ptr()->set_data(nullptr);
		// free_list에 삭제하고
		free_list.erase(Node);
		// 노드는 다시 넣어준다.
		node_pool.Free(Node.get_ptr());

		return obj;
	}

	void Free(T *Obj)
	{
		std::lock_guard<LOCK> g(mutex);

		// 링크를 하나 가져온다.
		cLinkedNode<T>* node = node_pool.Allocate();
		// Obj를 셋팅해준다.
		node->set_data(Obj);
		// free_list에 넣어준다.
		free_list.push_back(node);
	}

	virtual void *AllocVoid() {
		return Allocate();
	}

	virtual void FreeVoid(void *ptr) {
		Free((T*)ptr);
	}

};

