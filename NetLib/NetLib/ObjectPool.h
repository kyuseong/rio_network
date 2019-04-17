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

// ������ Object Pool
template<class T, class LOCK = NullMutex>
class ObjectPool : public BaseObjectPool
{
	FastFixedAllocator< LOCK > m_Alloc;
	
public:
	ObjectPool(size_t chunk_count = 32) : m_Alloc(sizeof(T), chunk_count)
	{
	}

	virtual ~ObjectPool() {}

	// �޸� �Ҵ�
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
	// ����
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

// �����ϴ� ������Ʈ Ǯ
template<class T, class LOCK = NullMutex>
class ReusableObjectPool : public BaseObjectPool
{
	// ������ �޸� Ǯ
	ObjectPool<T, NullMutex> objpool;
	// Node Pool (free_node ���� ������)
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

		// free_list�� ���� ������
		while (false == free_list.empty())
		{
			// top�� ���ؼ�
			auto top = free_list.begin();
			// node ������ ���ϰ�
			auto Node = top.get_ptr();
			// obj �� �������ְ�
			objpool.Free(top.get_ptr()->get_data());
			// free_list���� �����ϰ�
			free_list.erase(top);
			// Node�� Ǯ�� �ٽ� �־��ش�.
			node_pool.Free(Node);
		}
	}

	template<class... _Types>
	T * Allocate(_Types&&... _Args)
	{
		std::lock_guard<LOCK> g(mutex);
		// free_list �� �ֳ�?
		if (free_list.empty())
		{
			// ������ �׳� �Ҵ��ؼ� ��
			T* obj = objpool.Allocate(std::forward<_Types>(_Args)...);

			return obj;
		}

		// free_list�� ������ ó���� ������ �ش�.
		auto Node = free_list.begin();
		// obj�� ���ؿ´�.
		T*  obj = Node.get_ptr()->get_data();
		// null�� �������ְ�(Ȥ�� �𸣴ϱ�)
		Node.get_ptr()->set_data(nullptr);
		// free_list�� �����ϰ�
		free_list.erase(Node);
		// ���� �ٽ� �־��ش�.
		node_pool.Free(Node.get_ptr());

		return obj;
	}

	void Free(T *Obj)
	{
		std::lock_guard<LOCK> g(mutex);

		// ��ũ�� �ϳ� �����´�.
		cLinkedNode<T>* node = node_pool.Allocate();
		// Obj�� �������ش�.
		node->set_data(Obj);
		// free_list�� �־��ش�.
		free_list.push_back(node);
	}

	virtual void *AllocVoid() {
		return Allocate();
	}

	virtual void FreeVoid(void *ptr) {
		Free((T*)ptr);
	}

};

