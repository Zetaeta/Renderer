#include "DX12UploadBuffer.h"
#include "DX12Window.h"
#include <d3dx12.h>
#include "core/Utils.h"

namespace rnd::dx12
{

DX12UploadBuffer::DX12UploadBuffer(size_t startSize)
:mSize(startSize)
{
	CreateHeap(startSize);
}

void DX12UploadBuffer::CreateHeap(size_t size)
{
	ID3D12Device_* device = GetD3D12Device();

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	DXCALL(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadHeap)));
	CD3DX12_RANGE range(0, 0);
	DXCALL(mUploadHeap->Map(0, &range, reinterpret_cast<void**>(&mHeapData)));
}

DX12UploadBuffer::Allocation DX12UploadBuffer::Reserve(u64 size, u64 alignment)
{
	u64 result = 0;
	if (TryReserve(size, alignment, result))
	{
		return MakeAllocation(result);
	}

	// Throw away the old heap and create a new one large enough. This risks wasting memory for smaller uploads this frame
	// but whatever. TODO?
	ClearHeap();

	u64 newSize = RoundUpToPowerOf2(NumCast<u64>(max(size, mSize) * 1.5f));
	CreateHeap(newSize);
	CHECK_SUCCEEDED(TryReserve(size, alignment, result));
	return MakeAllocation(result);
}

bool DX12UploadBuffer::TryReserve(u64 size, u64 alignment, u64& outStart)
{
	if (size > mSize)
	{
		return false;
	}

	u64 currFrame = GetRHI().GetCurrentFrame();
	if (mFrames.empty())
	{
		mFrames.push({currFrame, 0, size});
		outStart = 0;
		return true;
	}

	FrameWindow& current = mFrames.back();
	u64 start = Align(current.End, alignment);
	u64 completedFrame = GetRHI().GetCompletedFrame();
	if (start + size > mSize)
	{
		while (!mFrames.empty() && mFrames.front().End > current.End)
		{
			if (completedFrame >= mFrames.front().Frame)
			{
				mFrames.pop();
			}
			else
			{
				return false;
			}
		}
		start = 0;
	}
	while (!mFrames.empty() && mFrames.front().Start < start + size)
	{
		if (completedFrame >= mFrames.front().Frame)
		{
			mFrames.pop();
		}
		else
		{
			return false;
		}
	}

	if (mFrames.empty())
	{
		mFrames.push({currFrame, 0, size});
		outStart = 0;
		return true;
	}
	mFrames.back().End = start + size;
	outStart = start;
	return true;
}

DX12UploadBuffer::~DX12UploadBuffer()
{
	ClearHeap();
}

void DX12UploadBuffer::ClearHeap()
{
	if (mUploadHeap)
	{
		ZE_ASSERT(mHeapData);
		mUploadHeap->Unmap(0, nullptr);
		mHeapData = nullptr;
		GetRHI().DeferredRelease(std::move(mUploadHeap));
		mUploadHeap = nullptr;
	}
}

void DX12UploadBuffer::ReleaseResources()
{
	ClearHeap();
}

}
