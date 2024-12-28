#include "DX12DescriptorHeap.h"

namespace rnd::dx12
{

DX12DescriptorHeap::DX12DescriptorHeap(ID3D12Device_* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size, D3D12_DESCRIPTOR_HEAP_FLAGS flags /*= D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
:Length(size), Type(type), Flags(flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = type;
	desc.NumDescriptors = size;
	desc.Flags = flags;
	HR_ERR_CHECK(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap)));
	ElementSize = pDevice->GetDescriptorHandleIncrementSize(type);
}

void DX12DescriptorHeap::Resize(u32 newLength, u32 copyLength, bool immediateRelease /*= false*/)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = Type;
	desc.NumDescriptors = newLength;
	desc.Flags = Flags;
	ComPtr<ID3D12DescriptorHeap> oldHeap = std::move(Heap);
	auto* device = GetD3D12Device();
	HR_ERR_CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap)));
	if (copyLength > 0)
	{
		device->CopyDescriptorsSimple(min(copyLength, Length), Heap->GetCPUDescriptorHandleForHeapStart(), oldHeap->GetCPUDescriptorHandleForHeapStart(), Type);
	}
	if (!immediateRelease)
	{
		GetRHI().DeferredRelease(std::move(oldHeap));
	}
}

void DX12DescriptorHeap::ReleaseResources()
{
	GetRHI().DeferredRelease(std::move(Heap));
	Heap = nullptr;
}

}
