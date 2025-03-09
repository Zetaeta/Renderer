#pragma once
#include "core/CoreTypes.h"

namespace rnd
{
template<size_t MaxFrames>
class ManualRingBuffer
{
public:
	ManualRingBuffer(u64 size)
		: mSize(size) {}
	void Reset(u64 size)
	{
		mSize = size;
		for (FrameWindow& frame : mFrames)
		{
			frame.Start = frame.End = 0;
		}
	}
	bool TryReserve(u64 size, u64 alignment, u64& outStart);
	struct FrameWindow
	{
		u64 Start;
		u64 End;
	};

	void PushFrame()
	{
		u64 start = 0;
		if (!mFrames.IsEmpty())
		{
			start = mFrames.Back().End;
		}
		mFrames.Push({start, start});
	}

	void PopFrame()
	{
		mFrames.Pop();
	}

	StaticQueue<FrameWindow, MaxFrames, EStaticQueueOverflowBehavior::Disallow> mFrames;
	u64 mSize = 0;
};

template <size_t MaxFrames>
bool ManualRingBuffer<MaxFrames>::TryReserve(u64 size, u64 alignment, u64& outStart)
{
	if (size > mSize)
	{
		return false;
	}

	FrameWindow& current = mFrames.Back();
	u64 start = Align(current.End, alignment);
	if (start + size > mSize)
	{
		start = 0;
	}

	if (!(mFrames.Front().Start >= start + size || mFrames.Front().End <= start))
	{
		return false;
	}

	mFrames.Back().End = start + size;
	outStart = start;
	return true;
}

}
