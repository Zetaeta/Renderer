#pragma once

#include "SharedD3D12.h"
#include "DX12DescriptorHeap.h"
#include <array>

namespace rnd::dx12
{

enum class EDescriptorType : u8
{
	SRV,
	UAV,
	CBV,
	Sampler,
	RTV,
	DSV,

	Count,
	ShaderVisibleCount = Sampler + 1,
	CbvSrvUavCount = CBV + 1,

};

class DX12DescriptorAllocator
{
public:
	virtual D3D12_CPU_DESCRIPTOR_HANDLE Allocate(EDescriptorType type) = 0;
	virtual void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	void ProcessDeferredReleases(u32 currentFrameIndex);
protected:
	Vector<D3D12_CPU_DESCRIPTOR_HANDLE> mFreeSpaces;
	std::array<Vector<D3D12_CPU_DESCRIPTOR_HANDLE>, MaxSwapchainBufferCount> mDeferredRelease; 
	u32 mNextFree = 0;
};

// Adds new heaps as required, rather than 1 big heap
class DX12SeparableDescriptorAllocator : public DX12DescriptorAllocator
{
public:
	DX12SeparableDescriptorAllocator(ID3D12Device_* device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 heapSize, bool isShaderVisible);
	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(EDescriptorType type) override;

	bool Accepts(EDescriptorType type) const;
protected:
	Vector<DX12DescriptorHeap> mHeaps;
	u32 mHeapSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE mType;
	bool mShaderVisible = true;
};

}
