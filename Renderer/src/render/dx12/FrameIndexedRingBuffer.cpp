#include "FrameIndexedRingBuffer.h"
#include "DX12RHI.h"

namespace rnd::dx12
{

bool FrameIndexedRingBuffer::TryReserve(u64 size, u64 alignment, u64& outStart)
{
//	ZE_ASSERT(IsInRenderThread());
	if (size > mSize)
	{
		return false;
	}

	u64 currFrame = GetRHI().GetCurrentFrame();
	if (mFrames.empty())
	{
		mFrames.Push({currFrame, 0, size});
		outStart = 0;
		return true;
	}

	FrameWindow& current = mFrames.Back();
	u64 start = Align(current.End, alignment);
	u64 completedFrame = GetRHI().GetCompletedFrame();
	if (start + size > mSize)
	{
		while (!mFrames.empty() && mFrames.Front().End > current.End)
		{
			if (completedFrame >= mFrames.Front().Frame)
			{
				mFrames.Pop();
			}
			else
			{
				return false;
			}
		}
		start = 0;
	}
	while (!mFrames.empty() && !(mFrames.Front().Start >= start + size || mFrames.Front().End <= start))
	{
		if (completedFrame >= mFrames.Front().Frame)
		{
			mFrames.Pop();
		}
		else
		{
			return false;
		}
	}

	if (mFrames.empty())
	{
		mFrames.Push({currFrame, 0, size});
		outStart = 0;
		return true;
	}
	mFrames.Back().End = start + size;
	outStart = start;
	return true;
}

void FrameIndexedRingBuffer::Reset(u64 size)
{
	mSize = size;
	mFrames = {};
}

}
