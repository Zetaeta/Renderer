#pragma once
#include "Types.h"
#include "Maths.h"
#include "Utils.h"

class MovableContiguousBumpAllocator
{
public:
	MovableContiguousBumpAllocator() {}
	MovableContiguousBumpAllocator(size_t initialCapacity)
		: mCapacity(initialCapacity), mData(MakeOwning<u8[]>(mCapacity))
	{
	}

	u8* Data()
	{
		return mData.get();
	}

	size_t AllocatedSize() { return mSize; }
	size_t Capacity() { return mCapacity; }

	u8* operator[](size_t index)
	{
		RASSERT(index < mSize);
		return &mData[index];
	}

	u8 const* operator[](size_t index) const
	{
		RASSERT(index < mSize);
		return &mData[index];
	}

	void Resize(size_t newCapacity)
	{
		RASSERT(newCapacity > mSize);
		OwningPtr<u8[]> newData = MakeOwning<u8[]>(newCapacity);
		memcpy(newData.get(), mData.get(), mSize);
		std::swap(newData, mData);
		mCapacity = newCapacity;
	}

	size_t Allocate(size_t size, size_t alignment = 0);

private:
	size_t mCapacity = 0;
	size_t mSize = 0;

	OwningPtr<u8[]> mData;
};
