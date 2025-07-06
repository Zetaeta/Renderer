#include "DX12SwapChain.h"
#include "render/DeviceTexture.h"
#include "editor/Viewport.h"
#include "DX12RHI.h"
#include "DX12DescriptorAllocator.h"

namespace rnd::dx12
{
constexpr DXGI_FORMAT gSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

DX12SwapChain::DX12SwapChain(wnd::Window* window, u32 numBuffers)
: IDeviceSurface(&GetRHI(), window), mNumBuffers(numBuffers), mSize(window->GetSize())
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = mNumBuffers;
	swapChainDesc.Width = mSize.x;
	swapChainDesc.Height = mSize.y;
	swapChainDesc.Format = gSwapChainFormat;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	ComPtr<IDXGISwapChain1> swapChain;
	auto* factory = GetRHI().GetFactory();
	HR_ERR_CHECK(factory->CreateSwapChainForHwnd(GetRHI().CmdQueues().Direct, window->GetHwnd(), &swapChainDesc, nullptr, nullptr, &swapChain));
	HR_ERR_CHECK(swapChain.As<IDXGISwapChain3>(&mSwapChain));
	factory->MakeWindowAssociation(window->GetHwnd(), DXGI_MWA_NO_ALT_ENTER);
	for (u32 i = 0; i < mNumBuffers; ++i)
	{
		mRtvDescriptors[i] = GetRHI().GetDescriptorAllocator(EDescriptorType::RTV)->Allocate(EDescriptorType::RTV);
	}

	GetBuffers();
}

void DX12SwapChain::Present()
{
	mSwapChain->Present(1, 0);
	mBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	for (auto const& vp : mViewports)
	{
		vp->SetBackbuffer(GetBackbuffer());
	}
}

void DX12SwapChain::RequestResize(uvec2 newSize)
{
	mResizeWidth = newSize.x;
	mResizeHeight = newSize.y;
}

void DX12SwapChain::GetBuffers()
{
	mBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	DeviceTextureDesc desc;
	desc.Width = mSize.x;
	desc.Height = mSize.y;
	desc.Format = ETextureFormat::RGBA8_Unorm;
	desc.Flags = TF_RenderTarget;
	desc.DebugName = "Backbuffer";

	// swapchain render targets
	{
		for (u32 i = 0; i < mNumBuffers; ++i)
		{
			HR_ERR_CHECK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
			GetD3D12Device()->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, mRtvDescriptors[i]);
			desc.DebugName = std::format("Backbuffer {}", i);
			mSwapchainBufferTextures[i] = std::make_shared<DX12Texture>(desc, mRenderTargets[i].Get(), mRtvDescriptors[i]);
		}
	}
}

void DX12SwapChain::Resize()
{
	mSize = {mResizeWidth, mResizeHeight};
	mResizeWidth = mResizeHeight = 0;
	for (u32 i = 0; i < mNumBuffers; ++i)
	{
		mRenderTargets[i] = nullptr;
		mSwapchainBufferTextures[i] = nullptr;
	}

	DXCALL(mSwapChain->ResizeBuffers(mNumBuffers, mSize.x, mSize.y, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	GetBuffers();
	if (mViewports.size() == 1)
	{
		mViewports[0]->Resize(mSize.x, mSize.y, mSwapchainBufferTextures[mBufferIndex]);
	}
}

void DX12SwapChain::ReleaseResourcesPreResize()
{
	for (auto const& vp : mViewports)
	{
		vp->Reset();
	}

	mSwapchainBufferTextures = {};
}

DXGI_FORMAT DX12SwapChain::GetRTVFormat() const
{
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

}
