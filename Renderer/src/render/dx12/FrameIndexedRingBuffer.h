#pragma once

#include "SharedD3D12.h"
#include <queue>
#include "container/StaticQueue.h"

template<typename T>
using Queue = std::queue<T>;

namespace rnd::dx12
{
class FrameIndexedRingBuffer
{
protected:
	FrameIndexedRingBuffer(u64 size = 0)
	:mSize(size)
	{
	}

	void Reset(u64 size);
	bool TryReserve(u64 size, u64 alignment, u64& outStart);
	struct FrameWindow
	{
		u64 Frame;
		u64 Start;
		u64 End;
	};
	StaticQueue<FrameWindow, 4, EStaticQueueOverflowBehavior::Evict> mFrames;
	u64 mSize = 0;
};

}
