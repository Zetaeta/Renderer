#pragma once

#include <array>
#include "mutex"
#include "SharedD3D12.h"

namespace rnd::dx12
{

class DX12CommandAllocatorPool
{
public:
	DX12CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE cmdListType)
		: mCmdListType(cmdListType) {}

	ComPtr<ID3D12CommandAllocator> Acquire();
	void Release(ComPtr<ID3D12CommandAllocator>&& allocator);

	void ProcessDeferredReleases(u32 frameIdx);

private:
	std::array<Vector<ComPtr<ID3D12CommandAllocator>>, MaxSwapchainBufferCount> mDeferredReleases;
	Vector<ComPtr<ID3D12CommandAllocator>> mPool;
	D3D12_COMMAND_LIST_TYPE mCmdListType;
	std::mutex mMutex;
};
}
