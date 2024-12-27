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
	DX12DescriptorHeap(ID3D12Device_* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
	:Length(size), Type(type)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc {};
		desc.Type = type;
		desc.NumDescriptors = size;
		desc.Flags = flags;
		HR_ERR_CHECK(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap)));
		ElementSize = pDevice->GetDescriptorHandleIncrementSize(type);
	}

	ID3D12DescriptorHeap* operator->()
	{
		return Heap.Get();
	}

	ComPtr<ID3D12DescriptorHeap> Heap;
	u32 Length = 0;
	u32 ElementSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE Type = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
};

}
}

