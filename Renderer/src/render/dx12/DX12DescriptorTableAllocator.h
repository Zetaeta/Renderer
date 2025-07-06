#pragma once

#include "SharedD3D12.h"

#include "FrameIndexedRingBuffer.h"
#include "DX12DescriptorHeap.h"

namespace rnd::dx12
{
struct DescriptorTableLoc
{
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	ID3D12DescriptorHeap* Heap;
};

class DX12DescriptorTableAllocator : FrameIndexedRingBuffer
{
public:
	DX12DescriptorTableAllocator(ID3D12Device_* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, u32 size = 2048);

	DescriptorTableLoc Reserve(u32 tableSize);
private:
	DX12DescriptorHeap mHeap;
};

}
