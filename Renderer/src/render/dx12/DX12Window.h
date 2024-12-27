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

namespace rnd
{
enum ESwapchainBufferCount : u8
{
	DoubleBuffered = 2,
	TripleBuffered = 3,
	MaxSwapchainBufferCount = TripleBuffered
};

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
private:

	void StartFrame();
	void EndFrame();
	void WaitForGPU();
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
	u64 mCurrentFenceValue = 0;
	std::array<u64, MaxSwapchainBufferCount> mFenceValues = {0,0,0};
	OwningPtr<DX12Test> mTest;
//	OwningPtr<DX12SyncPointPool> mSyncPoints;
	std::array<ComPtr<ID3D12Resource_>, MaxSwapchainBufferCount> mRenderTargets;
	DX12DescriptorHeap mRTVHeap;
	DX12CommandList mCmdList;
	ESwapchainBufferCount mNumBuffers;
	bool mClosed = false;
	u32 mFrameIndex = 0;
	u32 mResizeWidth = 0;
	u32 mResizeHeight = 0;
};

}
}
