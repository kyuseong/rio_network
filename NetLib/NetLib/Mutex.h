//
#pragma once 

#include <mutex>
#include <atomic>

// 
class NullMutex {
public:
	inline void lock() {}
	inline void unlock() {}
};

//  
class SpinLock {
	std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
	void lock() {
		for (int Try = 0;;++Try)
		{
			if (locked.test_and_set(std::memory_order_acquire) == false)
				break;

#ifdef min
#undef min
 			std::this_thread::sleep_for(std::chrono::milliseconds(std::min(5, Try)));
#endif
		}
	}
	bool try_lock()
	{
		return !locked.test_and_set(std::memory_order_acquire);
	}
	void unlock() {
		locked.clear(std::memory_order_release);
	}
	void clear()
	{
		locked.clear(std::memory_order_release);
	}
};


