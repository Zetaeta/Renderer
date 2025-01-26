#include "DX12CommandAllocatorPool.h"
#include "DX12RHI.h"

namespace rnd::dx12
{

ComPtr<ID3D12CommandAllocator> DX12CommandAllocatorPool::Acquire()
{
	{
		std::lock_guard lock(mMutex);
		if (!mPool.empty())
		{
			auto result = std::move(mPool.back());
			ZE_ASSERT(result);
			mPool.pop_back();
			return result;
		}
	}
	ComPtr<ID3D12CommandAllocator> result;
	DXCALL(GetD3D12Device()->CreateCommandAllocator(mCmdListType, IID_PPV_ARGS(&result)));
	return result;
}

void DX12CommandAllocatorPool::Release(ComPtr<ID3D12CommandAllocator>&& allocator)
{
	u32	frame = GetRHI().GetCurrentFrameIndex();
	std::lock_guard lock(mMutex);
	mDeferredReleases[frame].emplace_back(allocator);
}

void DX12CommandAllocatorPool::ProcessDeferredReleases(u32 frameIdx)
{
	std::lock_guard lock(mMutex);
	u32	frame = GetRHI().GetCurrentFrameIndex();
	mPool.reserve(mPool.size() + mDeferredReleases[frameIdx].size());
	for (auto& alloc : mDeferredReleases[frame])
	{
		alloc->Reset();
		mPool.push_back(std::move(alloc));
	}
	mDeferredReleases[frame].clear();
}

}
