#pragma once
#include <algorithm>
#include <vector>
#include <memory>
#include "core/TypeTraits.h"
#include "core/CoreTypes.h"

template<typename T>
using Vector = std::vector<T>;

using BitVector = std::vector<bool>;

template<template<typename T> typename Alloc = std::allocator>
struct StdAllocator
{
	constexpr static size_t StartingCapacity = 0;
	template<typename T>
	class Instance
	{
		Instance() = default;
		Instance(Alloc<T> const& inAllocInstance)
		:mBackingAllocator(inAllocInstance)
		{
		}


		T* Allocate(size_t count = 1)
		{
			return mBackingAllocator.allocate(count);
		}

		void Free(T* ptr, size_t n)
		{
			mBackingAllocator.deallocate(ptr, n);
		}

		Alloc<T> mBackingAllocator;
	};
};

namespace newvec
{
template<typename T, typename SizeType = u32, typename Allocator = StdAllocator<>>
class Vector
{
	Vector()
	{
		if constexpr (Allocator::StartingCapacity > 0)
		{
			mStart = mAllocator.Allocate(Allocator::StartingCapacity);
			mCapacity = Allocator::StartingCapacity;
		}
	}

	template<typename... Args>
	T& Emplace(Args... args)
	{
		T* newLocation = ExpandUninitialized(1);
		return *(new(newLocation) T(std::forward<Args>(args)...));
	}

	T* ExpandUninitialized(SizeType num)
	{
		if (mStart == nullptr || mSize + num > mCapacity)
		{
			GrowAllocation(mSize + num);
		}
		T* location = mStart + mSize;
		mSize += num;
		return location;
	}

private:

	void GrowAllocation(SizeType requiredCapacity)
	{
		SizeType desiredCapacity = NumCast<SizeType>(requiredCapacity * 1.5f);
		T* newData = mAllocator.Allocate(desiredCapacity);
		if constexpr (is_trivially_movable_v<T>)
		{
			memcpy(newData, mStart, mSize);
		}
		else
		{
			std::uninitialized_move_n(mStart, mSize, newData);
			std::destroy_n(mStart, mSize);
		}
		mAllocator.Free(mStart, mCapacity);
		mCapacity = desiredCapacity;
	}
	T* mStart = nullptr;
	SizeType mSize = 0;
	SizeType mCapacity = 0;
	Allocator::template Instance<T> mAllocator;
};

}

template<typename T, u32 Size>
using SmallVector = std::vector<T>;

template<typename T>
bool Remove(Vector<T>& vec, T const& value)
{
	if (auto it = std::find(vec.begin(), vec.end(), value); it != vec.end())
	{
		vec.erase(it);
		return true;
	}
	return false;
}
template<typename T>
bool RemoveSwap(Vector<T>& vec, T const& value)
{
	if (auto it = std::find(vec.begin(), vec.end(), value); it != vec.end())
	{
		*it = std::move(vec.back());
		vec.pop_back();
		return true;
	}
	return false;
}

template<typename T, typename Range>
T* Find(Range& range, T const& value)
{
	if (auto it = std::find(range.begin(), range.end(), value); it != range.end())
	{
		return &*it;
	}
	return nullptr;
}

template<typename T, typename Range>
bool Contains(Range& range, T const& value)
{
	if (auto it = std::find(range.begin(), range.end(), value); it != range.end())
	{
		return true;
	}
	return false;
}

template<typename Range, typename SizeType>
void RemoveSwapIndex(Range& range, SizeType index)
{
	std::swap(range.back(), range[index]);
	range.erase(range.end() - 1);
}
