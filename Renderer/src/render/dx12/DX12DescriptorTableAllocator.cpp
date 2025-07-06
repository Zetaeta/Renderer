#include "DX12DescriptorTableAllocator.h"
#include <d3dx12.h>

namespace rnd::dx12
{

DX12DescriptorTableAllocator::DX12DescriptorTableAllocator(ID3D12Device_* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, u32 size)
:FrameIndexedRingBuffer(size), mHeap(device, heapType, size, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
{
}

DescriptorTableLoc DX12DescriptorTableAllocator::Reserve(u32 tableSize)
{
	u64 offset;
	if (!TryReserve(tableSize, 1, offset))
	{
		mHeap.Resize(max(NumCast<u32>(mHeap.Length * 1.5f), tableSize), /*copyLength = */ 0);
		FrameIndexedRingBuffer::Reset(mHeap.Length);
		CHECK_SUCCEEDED(TryReserve(tableSize, 1, offset));
	}

	return 
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mHeap->GetGPUDescriptorHandleForHeapStart(), NumCast<int>(offset), mHeap.ElementSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeap->GetCPUDescriptorHandleForHeapStart(), NumCast<int>(offset), mHeap.ElementSize),
		mHeap.GetHeap()
	};
}

}
