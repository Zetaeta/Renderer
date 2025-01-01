#pragma once

#include "SharedD3D12.h"

namespace rnd
{
namespace dx12
{

class DX12DescriptorHeap
{
public:
	DX12DescriptorHeap() = default;
	DX12DescriptorHeap(ID3D12Device_* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	/**
	 * @param copyLength The number of descriptors (from the start) that should be copied across.
	 * If copyLength >= current length, all will be copied
	 */
	void Resize(u32 newLength, u32 copyLength = std::numeric_limits<u64>::max(), bool immediateRelease = false);

	void ReleaseResources();

	static u32 DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	static void GetDescriptorSizes(ID3D12Device_* device);

	ID3D12DescriptorHeap* operator->()
	{
		return Heap.Get();
	}
	ID3D12DescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}

	ComPtr<ID3D12DescriptorHeap> Heap;
	u32 Length = 0;
	u32 ElementSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE Type = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
};

}
}

