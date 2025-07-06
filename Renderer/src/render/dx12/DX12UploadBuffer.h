#pragma once

#include "SharedD3D12.h"
#include <d3dx12.h>
#include <queue>
#include "FrameIndexedRingBuffer.h"

namespace rnd::dx12
{

/**
 * A ring-buffered upload heap
 */
 template<typename RingBufferType = FrameIndexedRingBuffer>
class TDX12RingBuffer : protected RingBufferType
{
public:
	TDX12RingBuffer() = default;
	TDX12RingBuffer(size_t startSize, D3D12_HEAP_TYPE type = D3D12_HEAP_TYPE_UPLOAD)
	:RingBufferType(startSize), mType(type)
	{
		CreateBuffer(startSize);
	}
	virtual ~TDX12RingBuffer()
	{
		ClearBuffer();
	}
	ZE_COPY_PROTECT(TDX12RingBuffer);
	RMOVE_DEFAULT(TDX12RingBuffer);

	void ReleaseResources()
	{
		ClearBuffer();
	}

	struct Allocation
	{
		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
		u64 Offset;
		// Write address may be invalidated by any subsequent calls to Reserve
		void* WriteAddress; 
	};

	Allocation Reserve(u64 size, u64 alignment)
	{
		ZE_ASSERT(size > 0);
		u64 result = 0;
		if (RingBufferType::TryReserve(size, alignment, result))
		{
			return MakeAllocation(result);
		}

		// Throw away the old heap and create a new one large enough. This risks wasting memory for smaller uploads this frame
		// but whatever. TODO?
		ClearBuffer();

		u64 newSize = RoundUpToPowerOf2(NumCast<u64>(max(size, this->mSize) * 1.5f));
		RingBufferType::Reset(newSize);
		CreateBuffer(newSize);
		CHECK_SUCCEEDED(RingBufferType::TryReserve(size, alignment, result));
		return MakeAllocation(result);
	}

	ID3D12Resource* GetCurrentBuffer()
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
		return {mUploadHeap->GetGPUVirtualAddress() + offset, offset, GetWriteAddress(offset)};
	}
	void CreateBuffer(size_t size)
	{
		ID3D12Device_* device = GetD3D12Device();

		CD3DX12_HEAP_PROPERTIES heapProps(mType);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
		if (mType == D3D12_HEAP_TYPE_READBACK)
		{
			state = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		DXCALL(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, state, nullptr, IID_PPV_ARGS(&mUploadHeap)));
		CD3DX12_RANGE range(0, 0);
		DXCALL(mUploadHeap->Map(0, &range, reinterpret_cast<void**>(&mHeapData)));
	}

	void ClearBuffer()
	{
		if (mUploadHeap)
		{
			ZE_ASSERT(mHeapData);
			mUploadHeap->Unmap(0, nullptr);
			mHeapData = nullptr;
			DX12DeferredRelease(std::move(mUploadHeap)); // TODO Variable release behavior 
			mUploadHeap = nullptr;
		}
	}
	ComPtr<ID3D12Resource> mUploadHeap;

	u8* mHeapData = nullptr;
	D3D12_HEAP_TYPE mType = D3D12_HEAP_TYPE_UPLOAD;
};

using DX12UploadBuffer = TDX12RingBuffer<FrameIndexedRingBuffer>;

}
