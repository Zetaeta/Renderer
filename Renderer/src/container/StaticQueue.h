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

	template<bool IsConst>
	class BaseIterator
	{
	public:
		using Target = ApplyConst<T, IsConst>;
		using Owner = ApplyConst<StaticQueue, IsConst>;
		BaseIterator(Owner& owner, u32 position)
			: mOwner(owner), mPosition(position)
		{
		}
		BaseIterator& operator++()
		{
			if (++mPosition > InCapacity)
			{
				mPosition = 0;
			}
			return *this;
		}

		BaseIterator operator++(int)
		{
			auto result = *this;
			++(*this);
			return result;
		}

		Target& operator*() const
		{
			ZE_ASSERT(mOwner.IsValidIndex(mPosition));
			return mOwner.mData[mPosition].Launder();
		}

		Target* operator->() const
		{
			return &(**this);
		}

		constexpr bool operator==(BaseIterator const& other) const
		{
			Assertf(mOwner == other.mOwner, "Comparing iterators belonging to different objects!");
			return mPosition == other.mPosition;
		}

		constexpr bool operator!=(BaseIterator const& other) const
		{
			return !(mPosition == other.mPosition);
		}

	private:
		Owner& mOwner;
		u32 mPosition = 0;
	};
	template<bool IsConst>
	friend class BaseIterator;

	using Iterator = BaseIterator<false>;
	using ConstIterator = BaseIterator<true>;

	Iterator begin()
	{
		return Iterator(*this, mStart);
	}

	Iterator end()
	{
		return Iterator(*this, mStart + mSize % InCapacity);
	}

	ConstIterator begin() const
	{
		return ConstIterator(*this, mStart);
	}

	ConstIterator end() const
	{
		return ConstIterator(*this, mStart + mSize % InCapacity);
	}
	
private:

	bool IsValidIndex(u32 index) const
	{
		return (index >= mStart && index - mStart < mSize) || (mStart + mSize > InCapacity && index < mStart + mSize - InCapacity);
	}
	std::array<UninitializedMemory<T>, InCapacity> mData;
	u32 mStart = 0;
	u32 mSize = 0;
};
