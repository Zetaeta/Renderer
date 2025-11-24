#include "FrameIndexedRingBuffer.h"
#include "DX12RHI.h"

namespace rnd::dx12
{

#define FIRB_DEBUG 1  

bool FrameIndexedRingBuffer::TryReserve(u64 size, u64 alignment, u64& outStart)
{
//	ZE_ASSERT(IsInRenderThread());
	if (size > mSize)
	{
		return false;
	}
	ZE_ASSERT(size > 0);

	u64 currFrame = GetRHI().GetCurrentFrame();
	if (mFrames.empty())
	{
		mFrames.Push({currFrame, 0, size});
		outStart = 0;
		return true;
	}

	FrameWindow& current = mFrames.Back();
	u64 start = Align(current.End, alignment);
	u64 completedFrame = GetRHI().GetCompletedFrame();// - 1;

	while (!mFrames.empty() && mFrames.Front().Frame <= completedFrame)
	{
		mFrames.Pop();
	}

	if (start + size > mSize)
	{
		start = 0;
	}
	u64 end = start + size;
	if (!mFrames.empty())
	{
		u64 allocStart = mFrames.Front().Start, allocEnd = mFrames.Back().End;
		bool bFull;
		if (allocEnd >= allocStart)
		{
			bFull = end > allocStart && start < allocEnd;
		}
		else
		{
			bFull = end > allocStart || start < allocEnd;
		}
		if (bFull) return false;
	}

#if FIRB_DEBUG
	
	for (auto it = mFrames.begin(); it != mFrames.end(); it++)
	{
		if (end > it->Start && start < it->End)
		{
			ZE_ASSERT(false);
		}
	}

#endif

	if (mFrames.empty() || mFrames.Back().Frame != currFrame)
	{
		if (mFrames.Capacity() == mFrames.Size()) ZE_ASSERT(mFrames.Front().Frame <= completedFrame);
		mFrames.Push({currFrame, start, start + size});
	}
	else
	{
		mFrames.Back().End = start + size;
	}

	outStart = start;
	return true;
}

void FrameIndexedRingBuffer::Reset(u64 size)
{
	mSize = size;
	mFrames = {};
}

}
