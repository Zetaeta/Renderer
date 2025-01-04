#pragma once
#include "core/memory/BaseUtils.h"

enum class EStaticQueueOverflowBehavior : u8
{
	Disallow, // asserts on attempted overfilling
	Evict // Silently pops the front to make space
};

/**
 * Queue implemented as a ring buffer using inline storage.
 * Any capacity is supported, but powers of two are preferred as the compiler can optimize % operator
 */
template<typename T, u32 InCapacity, EStaticQueueOverflowBehavior OverflowBehavior = EStaticQueueOverflowBehavior::Disallow>
class StaticQueue
{
public:
	template<typename... Args>
	T& Emplace(Args... args)
	{
		if constexpr (OverflowBehavior == EStaticQueueOverflowBehavior::Disallow)
		{
			ZE_ASSERT(mSize < InCapacity);
		}
		else
		{
			if (mSize == InCapacity)
			{
				Pop();
			}
		}
		u32 newLoc = (mStart + mSize) % InCapacity;
		mSize++;
		return mData[newLoc].Construct(std::forward<Args>(args)...);
	}

	T& Push(T&& val)
	{
		return Emplace(std::move(val));
	}

	T& Push(T const& val)
	{
		return Emplace(val);
	}

	T& Front()
	{
		ZE_ASSERT(mSize > 0);
		return mData[mStart].Launder();
	}

	T const& Front() const
	{
		ZE_ASSERT(mSize > 0);
		return mData[mStart].Launder();
	}

	T& Back()
	{
		ZE_ASSERT(mSize > 0);
		return mData[(mStart + mSize - 1) % InCapacity].Launder();
	}

	T const& Back() const
	{
		ZE_ASSERT(mSize > 0);
		return mData[(mStart + mSize - 1) % InCapacity].Launder();
	}

	T Pop()
	{
		ZE_ASSERT(mSize > 0);
		T result = std::move(mData[mStart].Launder());
		mData[mStart].Destruct();
		mStart = (mStart + 1) % InCapacity;
		mSize--;
		return result;
	}

	bool IsEmpty() const
	{
		return mSize == 0;
	}

	bool empty() const
	{
		return IsEmpty();
	}

	u32 Size() const
	{
		return mSize;
	}

	u32 Capacity() const
	{
		return InCapacity;
	}
private:
	std::array<UninitializedMemory<T>, InCapacity> mData;
	u32 mStart = 0;
	u32 mSize = 0;
};
