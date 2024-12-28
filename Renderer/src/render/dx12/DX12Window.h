#pragma once

#include "core/Maths.h"
#include "core/WinUtils.h"
#include "core/Types.h"
#include "platform/windows/Window.h"
#include "platform/windows/WinApi.h"
#include "SharedD3D12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <array>
#include "DX12DescriptorHeap.h"
#include "DX12SyncPoint.h"
#include "DX12CBPool.h"

namespace rnd
{

namespace dx12
{

class DX12Test;

struct DX12CommandList
{
	ComPtr<ID3D12GraphicsCommandList_> CmdList;
	std::array<ComPtr<ID3D12CommandAllocator>, MaxSwapchainBufferCount> Allocators;
};

class DX12Window : public wnd::Window
{
public:
	DX12Window(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers);
	~DX12Window();

	void Tick() override;

	u64 GetCompletedFrame() const;
	u64 GetCurrentFrame() const
	{
//		return mFra
		return mCurrentFrame;
	}
	// Returns current frame % num buffers
	u32 GetCurrentFrameIndex() const
	{
		return mFrameIndex;
	}
	void WaitForGPU();


	void DeferredRelease(ComPtr<ID3D12Pageable>&& resource);

	void ProcessDeferredRelease(u32 frameIndex);

	ID3D12Device_* Device() const
	{
		return mDevice.Get();
	}

	u32 GetMaxActiveFrames() const
	{
		return mNumBuffers;
	}

	DX12CBPool& GetCBPool()
	{
		return mCBPool;
	}
private:
	void WaitFence(u64 value);

	void StartFrame();
	void EndFrame();
	void Resize_WndProc(u32 resizeWidth, u32 resizeHeight) override;
	void OnDestroy_WndProc() override;

	void CreateDeviceAndCmdQueue();
	void ResizeSwapChain();
	void GetSwapChainBuffers();


	ComPtr<ID3D12CommandQueue> mCmdQueue;
	ComPtr<ID3D12Device_> mDevice; 
	ComPtr<IDXGISwapChain3> mSwapChain; 
	ComPtr<ID3D12Fence> mFrameFence;
	HANDLE mFenceEvent{};
	u64 mCurrentFrame = 0;
	std::array<u64, MaxSwapchainBufferCount> mFenceValues = {0,0,0};
	OwningPtr<DX12Test> mTest;
//	OwningPtr<DX12SyncPointPool> mSyncPoints;
	std::array<ComPtr<ID3D12Resource_>, MaxSwapchainBufferCount> mRenderTargets;
	std::array<Vector<ComPtr<ID3D12Pageable>>, MaxSwapchainBufferCount> mDeferredReleaseResources;
	DX12DescriptorHeap mRTVHeap;
	DX12CommandList mCmdList;
	ESwapchainBufferCount mNumBuffers;
	bool mClosed = false;
	u32 mFrameIndex = 0;
	u32 mBufferIndex = 0;
	u32 mResizeWidth = 0;
	u32 mResizeHeight = 0;

	DX12CBPool mCBPool;
};

using DX12RHI = DX12Window;

ID3D12Device_* GetD3D12Device();
DX12RHI& GetRHI();

}
}
