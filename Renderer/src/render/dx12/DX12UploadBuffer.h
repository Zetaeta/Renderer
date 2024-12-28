#pragma once

#include "SharedD3D12.h"
#include <queue>

using Queue = std::queue;

namespace rnd::dx12
{

/**
 * A ring-buffered upload heap
 */
class DX12UploadBuffer
{
public:
	DX12UploadBuffer() = default;
	DX12UploadBuffer(size_t startSize);
	~DX12UploadBuffer();
	RCOPY_PROTECT(DX12UploadBuffer);
	RMOVE_DEFAULT(DX12UploadBuffer);

	void ReleaseResources();

	struct Allocation
	{
		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
		// Write address may be invalidated by any subsequent calls to Reserve
		void* WriteAddress; 
	};

	Allocation Reserve(u64 size, u64 alignment);

	ID3D12Resource* GetCurrentHeap()
	{
		return mUploadHeap.Get();
	}

	// In case the heap is 
protected:

	u8* GetWriteAddress(u64 offset = 0) const
	{
		return mHeapData + offset;
	}
	Allocation MakeAllocation(u64 offset)
	{
		return {mUploadHeap->GetGPUVirtualAddress() + offset, GetWriteAddress(offset)};
	}
	void CreateHeap(size_t size);
	void ClearHeap();
	bool TryReserve(u64 size, u64 alignment, u64& outStart);
	ComPtr<ID3D12Resource> mUploadHeap;

	struct FrameWindow
	{
		u64 Frame;
		u64 Start;
		u64 End;
	};
	std::queue<FrameWindow> mFrames;
	u64 mSize = 0;
	u8* mHeapData = nullptr;
};

}
