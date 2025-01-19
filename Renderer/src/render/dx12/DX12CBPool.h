#pragma once
#include "render/RenderDevice.h"
#include "DX12UploadBuffer.h"
#include "DX12DescriptorHeap.h"
#include "render/ConstantBuffer.h"

// Whether to create CBV descriptors or just always use root signature CBVs
#ifndef DX12_USE_CBV_DESCRIPTORS
#define DX12_USE_CBV_DESCRIPTORS 0
#endif

namespace rnd::dx12
{

class DX12CBPool : public ICBPool
{
public:
	DX12CBPool() = default;
	DX12CBPool(DX12RHI& rhi);
	PooledCBHandle AcquireConstantBuffer(ECBLifetime lifetime, u32 size, std::span<const byte> initialData) override;
	using ICBPool::AcquireConstantBuffer;

	void ReleaseConstantBuffer(PooledCBHandle handle) override;

	void ReleaseResources();

private:
	DX12UploadBuffer mDynamicCBHeap;
#if DX12_USE_CBV_DESCRIPTORS
	std::array<DX12DescriptorHeap, MaxSwapchainBufferCount> mDynamicDescriptorHeaps;
#endif
	u32 mFrameIdx = 0;
	u32 mNextDynDescPos = 0;
//	DX12DescriptorHeap mCBHeap;
	
//	union CBIndex
	struct CBHandleStatic
	{
		u64 IsDynamic : 1;
		u64 StaticIdx : 63;
	};

	struct CBHandleDynamic
	{
		u64 IsDynamic: 1;
		u64 FrameIdx : 2;
		u64 CBIdx : 61;
	};
	union Handle
	{
		u64 Bits;
		CBHandleStatic Static;
		CBHandleDynamic Dynamic;
	};
};

class DX12DynamicCB : public IConstantBuffer
{
public:
	void Update() override
	{
		mDirty = true;
	}

	void SetLayout(DataLayout::Ref layout);

	D3D12_GPU_VIRTUAL_ADDRESS Commit();

	bool mDirty = true;
	u64 mCurrentFrame = 0;
	D3D12_GPU_VIRTUAL_ADDRESS  mLastAddress{};
};
	
}
