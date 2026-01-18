#include <d3dx12.h>

#include "DX12CBPool.h"

#include "DX12RHI.h"

namespace rnd::dx12
{

constexpr size_t ConstantBufferAlignment = 256;
PooledCBHandle DX12CBPool::AcquireConstantBuffer(ECBLifetime lifetime, u32 size, std::span<const byte> initialData)
{
	size = RoundUpToMultipleP2(size, 256u);
	u32 currentFrameIdx = GetRHI().GetCurrentFrameIndex();
	if (mFrameIdx != currentFrameIdx)
	{
		mFrameIdx = currentFrameIdx;
		mNextDynDescPos = 0;
	}
	if (lifetime == ECBLifetime::Dynamic)
	{
		DX12UploadBuffer::Allocation alloc = mDynamicCBHeap.Reserve(size, ConstantBufferAlignment);
//		GetD3D12Device()->CreatePlacedResource(mDynamicCBHeap.GetCurrentHeap(), alloc.)
		memcpy(alloc.WriteAddress, initialData.data(), initialData.size());
#if DX12_USE_CBV_DESCRIPTORS
		auto& descHeap = mDynamicDescriptorHeaps[mFrameIdx];
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
		cbDesc.BufferLocation = alloc.GPUAddress;
		cbDesc.SizeInBytes = size;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descHeap->GetCPUDescriptorHandleForHeapStart(), mNextDynDescPos, descHeap.ElementSize);
		GetD3D12Device()->CreateConstantBufferView(&cbDesc, cpuHandle);
#endif
		Handle hdl;
		hdl.Dynamic.CBIdx = mNextDynDescPos;
		hdl.Dynamic.FrameIdx = currentFrameIdx;
		hdl.Dynamic.IsDynamic = true;
		PooledCBHandle result;
//		result.UserData.As<D3D12_CPU_DESCRIPTOR_HANDLE>() = cpuHandle;
		result.UserData.As<D3D12_GPU_VIRTUAL_ADDRESS>() = alloc.GPUAddress;
		result.Id = hdl.Bits;
		return result;
//		return PooledCBHandle(hdl.Bits);
	}
	ZE_ASSERT(false);
	return {};
}

void DX12CBPool::ReleaseConstantBuffer(PooledCBHandle handle)
{
	Handle hdl;
	hdl.Bits = handle.Id;
	if (hdl.Dynamic.IsDynamic)
	{
		// Dynamic cbs don't need releasing
	}
	else
	{
		ZE_ASSERT(false);
	}
}

DX12CBPool::DX12CBPool(DX12RHI& rhi)
:mDynamicCBHeap(1024*1024)
{

#if DX12_USE_CBV_DESCRIPTORS
	ID3D12Device_* device = rhi.Device();
	constexpr u64 DynamicCBHeapStartSize = 1024 * 64;
	for (u32 i = 0; i < rhi.GetMaxActiveFrames(); ++i)
	{
		mDynamicDescriptorHeaps[i] = DX12DescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DynamicCBHeapStartSize);
	}
#endif
}

void DX12CBPool::ReleaseResources()
{
	mDynamicCBHeap.ReleaseResources();
}

D3D12_GPU_VIRTUAL_ADDRESS DX12DynamicCB::Commit()
{
	auto& rhi = GetRHI();
	u64 thisFrame = rhi.GetCurrentFrame();
	if (mDirty || mCurrentFrame != thisFrame)
	{
		mDirty = false;
		mLastAddress = rhi.GetCBPool().AcquireConstantBuffer(ECBLifetime::Dynamic, mData.GetView())
							.UserData.As<D3D12_GPU_VIRTUAL_ADDRESS>();
		mCurrentFrame = thisFrame;
	}

	return mLastAddress;
}

void DX12DynamicCB::SetLayout(DataLayout::Ref layout)
{
	if (layout && layout->GetSize() < GetCBData().GetSize())
	{
		GetCBData().Resize(layout->GetSize());
	}
	GetCBData().SetLayout(layout);
}

}
