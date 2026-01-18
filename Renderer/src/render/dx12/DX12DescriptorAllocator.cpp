#include <d3dx12.h>

#include "DX12DescriptorAllocator.h"
#include "DX12RHI.h"

namespace rnd::dx12
{

// Base class
void DX12DescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	mDeferredRelease[GetRHI().GetCurrentFrameIndex()].push_back(handle);
}

void DX12DescriptorAllocator::ProcessDeferredReleases(u32 currentFrameIndex)
{
	mFreeSpaces.insert(mFreeSpaces.end(), mDeferredRelease[currentFrameIndex].begin(), mDeferredRelease[currentFrameIndex].end());
	mDeferredRelease[currentFrameIndex].clear();
}

// Separable
DX12SeparableDescriptorAllocator::DX12SeparableDescriptorAllocator(ID3D12Device_* device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 heapSize, bool isShaderVisible)
:mHeapSize(heapSize), mType(type), mShaderVisible(isShaderVisible)
{
	mHeaps.emplace_back(device, type, heapSize,
		isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12SeparableDescriptorAllocator::Allocate(EDescriptorType type)
{
	ZE_ASSERT(Accepts(type));
	if (!mFreeSpaces.empty())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE result = mFreeSpaces.back();
		mFreeSpaces.pop_back();
		return result;
	}

	if (mNextFree < mHeapSize)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeaps.back()->GetCPUDescriptorHandleForHeapStart(), mNextFree++, mHeaps.back().ElementSize);
	}

	mHeaps.emplace_back(GetD3D12Device(), mType, mHeapSize,
		mShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	mNextFree = 1;
	return mHeaps.back()->GetCPUDescriptorHandleForHeapStart();
}

bool DX12SeparableDescriptorAllocator::Accepts(EDescriptorType type) const
{
	switch (mType)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return type < EDescriptorType::CbvSrvUavCount;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		return type == EDescriptorType::Sampler;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		return type == EDescriptorType::RTV;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return type == EDescriptorType::DSV;
	default:
		return false;
	}
}

}
