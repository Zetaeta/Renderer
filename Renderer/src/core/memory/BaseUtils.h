#pragma once
#include "core/Utils.h"

using byte = unsigned char;

template<bool IsTrackingEnabled, typename SizeType = u32>
class PoolUsageTracker
{
protected:
	void TrackUsed(SizeType index, bool isUsed)
	{
	}
	void GrowUsedTracker(SizeType newSize) {}

	void CheckUsed(SizeType index) const {}
};

template<typename SizeType>
class PoolUsageTracker<true, SizeType>
{
protected:
	void TrackUsed(SizeType index, bool isUsed)
	{
		mInUse[index] = isUsed;
	}

	void GrowUsedTracker(SizeType newSize)
	{
		mInUse.resize(newSize + mInUse.size());
	}

	void CheckUsed(SizeType index) const
	{
		ZE_ASSERT(mInUse[index]);
	}

	bool IsInUse(SizeType index)
	{
		return mInUse[index];
	}


	Vector<bool> mInUse;
};

template<typename T>
struct UninitializedMemory
{
	alignas(T)
	byte Data[sizeof(T)];

	UninitializedMemory()
	{
	#if ZE_DEBUG_BUILD
		memset(Data, GARBAGE_VALUE, sizeof(T));
	#endif
	}
	UninitializedMemory(UninitializedMemory const& other)
		requires std::is_trivially_copyable_v<T>
	{
		memcpy(Data, other.Data, sizeof(T));
	}

	UninitializedMemory& operator=(UninitializedMemory const& other)
		requires std::is_trivially_copyable_v<T>
	{
		memcpy(Data, other.Data, sizeof(T));
		return *this;
	}

	T& Launder()
	{
		return *std::launder<T>(reinterpret_cast<T*>(&Data[0]));
	}

	T const& Launder() const
	{
		return *std::launder<T const>(reinterpret_cast<T const*>(&Data[0]));
	}

	template<typename... Args>
	T& Construct(Args... args)
	{
		return *(new (&Data[0]) T(std::forward<Args>(args)...));
	}

	void Destruct()
	{
		Launder().~T();
	#if ZE_DEBUG_BUILD
		memset(Data, GARBAGE_VALUE, sizeof(T));
	#endif
	}
};

struct DefaultDestruct
{
	template<typename T>
	void operator()(T& val)
	{
		val.~T();
	}
};

template<typename T>
struct DefaultConstruct
{
	T& operator()(void* location)
	{
		return *(new (location) T);
	}
};

inline void* AlignedAllocate(size_t size, size_t alignment)
{
	return ::operator new(size, (std::align_val_t)alignment);
}

inline void AlignedFree(void* ptr, size_t size, size_t alignment)
{
	::operator delete(ptr, (std::align_val_t) alignment);
}
