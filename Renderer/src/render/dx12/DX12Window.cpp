#include "DX12Window.h"

#include "core/WinUtils.h"
#include <d3d12sdklayers.h>
#include "dxgi1_6.h"
#include <d3dx12.h>
#include <d3d12sdklayers.h>
#include "core/Utils.h"
#include "DX12DescriptorHeap.h"
#include "DX12Test.h"

#pragma comment(lib, "d3d12.lib")

namespace rnd
{
namespace dx12
{

DX12Window::DX12Window(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers)
:Window(width, height, name), mNumBuffers(numBuffers)
{
	//WNDCLASSEX wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DX12 window", nullptr };
	//ZE_ASSERT(RegisterClassEx(&wc) != 0);
	//mHwnd = ::CreateWindowW(wc.lpszClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, width, height, nullptr, nullptr, wc.hInstance, nullptr);
	//ZE_ASSERT(mHwnd != 0);

	//ShowWindow(mHwnd, SW_SHOWDEFAULT);
	//::UpdateWindow(mHwnd);

	CreateDeviceAndCmdQueue();

	DX12SyncPointPool::Create(mDevice.Get());

	mFenceEvent = CreateEventW(nullptr, false, false, nullptr);

	WaitForGPU();
}

DX12Window::~DX12Window()
{
	DX12SyncPointPool::Destroy();
}

void DX12Window::Tick()
{
	if (mClosed)
	{
		return;
	}
	if (mResizeHeight != 0)
	{
		ResizeSwapChain();
	}
	StartFrame();
	auto& cmd = mCmdList.CmdList;
	cmd->Reset(mCmdList.Allocators[mFrameIndex].Get(), mTest->mPSO.Get());

	D3D12_VIEWPORT vp {};
	vp.Height = float(mHeight);
	vp.Width = float(mWidth);
	vp.TopLeftX = vp.TopLeftY = 0;
	vp.MaxDepth = 1;
	vp.MinDepth = 0;
	cmd->RSSetViewports(1, &vp);
	CD3DX12_RECT scissor(0, 0, mWidth, mHeight);
	cmd->RSSetScissorRects(1, &scissor);

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd->ResourceBarrier(1, &barrier);
	}
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRTVHeap.ElementSize);
	cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
	float const clearColor[4] = {0.5f, 0.5f, 0.5f, 1.f};
	cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	mTest->Render(*cmd.Get());
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmd->ResourceBarrier(1, &barrier);
	}
	cmd->Close();
	ID3D12CommandList* const cmdList = cmd.Get();
	mCmdQueue->ExecuteCommandLists(1, &cmdList);
	EndFrame();
}

void DX12Window::StartFrame()
{
	if (mFrameFence->GetCompletedValue() < mFenceValues[mFrameIndex])
	{
		HR_ERR_CHECK(mFrameFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}
	mFenceValues[mFrameIndex] = mCurrentFenceValue + 1;
	//if (mFenceValue > 2)
	//{

	//}
}

void DX12Window::EndFrame()
{
	HR_ERR_CHECK(mSwapChain->Present(1, 0));
	mCurrentFenceValue = mFenceValues[mFrameIndex];

	HR_ERR_CHECK(mCmdQueue->Signal(mFrameFence.Get(), mCurrentFenceValue));
	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void DX12Window::WaitForGPU()
{
	DX12SyncPoint syncPoint = DX12SyncPointPool::Get()->Claim();
	syncPoint.GPUSignal(mCmdQueue.Get());
	syncPoint.Wait();
	syncPoint.Release();
	//++mCurrentFenceValue;
	//HR_ERR_CHECK(mCmdQueue->Signal(mFrameFence.Get(), mCurrentFenceValue));
	//HR_ERR_CHECK(mFrameFence->SetEventOnCompletion(mCurrentFenceValue, mFenceEvent));
	//WaitForSingleObject(mFenceEvent, INFINITE);
	//mFenceValues[mFrameIndex] = mCurrentFenceValue;

}

void DX12Window::Resize_WndProc(u32 resizeWidth, u32 resizeHeight)
{
	mResizeWidth = resizeWidth;
	mResizeHeight = resizeHeight;
}

void DX12Window::OnDestroy_WndProc()
{
	wnd::Window::OnDestroy_WndProc();
	mClosed = true;
}

void DX12Window::CreateDeviceAndCmdQueue()
{
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif


	ComPtr<IDXGIFactory6> factory;
	HR_ERR_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> adapter;

	for (u32 adapterIdx = 0; SUCCEEDED(factory->EnumAdapterByGpuPreference(adapterIdx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); ++adapterIdx)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice))))
		{
			break;
		}
	}
	ZE_ASSERT(mDevice);

	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(mDevice.As<ID3D12InfoQueue>(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HR_ERR_CHECK(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = mNumBuffers;
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
//	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	ComPtr<IDXGISwapChain1> swapChain;
	HR_ERR_CHECK(factory->CreateSwapChainForHwnd(mCmdQueue.Get(), mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain));
	HR_ERR_CHECK(swapChain.As<IDXGISwapChain3>(&mSwapChain));
	factory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER);

	// Create command list + allocators
	HR_ERR_CHECK(mDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCmdList.CmdList)));
	for (u32 i = 0; i < mNumBuffers; ++i)
	{
		HR_ERR_CHECK(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdList.Allocators[i])));
	}

	GetSwapChainBuffers();

	mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence));


//	HR_ERR_CHECK(m)
}

void DX12Window::ResizeSwapChain()
{
	WaitForGPU();
	mTest = nullptr;
	mRTVHeap = {};
	mWidth = mResizeWidth;
	mHeight = mResizeHeight;
	mResizeWidth = mResizeHeight = 0;
	for (int i = 0; i < mNumBuffers; ++i)
	{
		mRenderTargets[i] = nullptr;
	}

	mSwapChain->ResizeBuffers(mNumBuffers, mWidth, mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	GetSwapChainBuffers();
}

void DX12Window::GetSwapChainBuffers()
{

	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
	mRTVHeap = DX12DescriptorHeap(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mNumBuffers);

	// swapchain render targets
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{mRTVHeap->GetCPUDescriptorHandleForHeapStart()};
		for (u32 i = 0; i < mNumBuffers; ++i)
		{
			HR_ERR_CHECK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
			mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, mRTVHeap.ElementSize);
		}
	}
	mTest = MakeOwning<DX12Test>(mDevice.Get());
}

}
}
