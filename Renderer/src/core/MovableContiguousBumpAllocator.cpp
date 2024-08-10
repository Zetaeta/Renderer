#include "MovableContiguousBumpAllocator.h"



size_t MovableContiguousBumpAllocator::Allocate(size_t size, size_t alignment /*= 0*/)
{
	size_t allocStart = mSize;
	if (alignment > 0)
	{
		size_t const misalignment = allocStart % alignment;
		allocStart += alignment - misalignment;
	}
	if (allocStart + mSize > mCapacity)
	{
		Resize(max(mCapacity * 2, allocStart + mSize));
	}
	memset(&mData[allocStart], 0, size);
	mSize = allocStart + size;
	return allocStart;
}
