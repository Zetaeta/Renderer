#include "DX11Swapchain.h"
#include "editor/Viewport.h"
#include "dxgi1_2.h"
#include "DX11Device.h"
#include "platform/windows/Window.h"

namespace rnd::dx11
{

DX11SwapChain::DX11SwapChain(DX11Device* device, wnd::Window* window)
:IDeviceSurface(device, window)
{
	mSize = window->GetSize();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = mSize.x;
	swapChainDesc.Height = mSize.y;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	IDXGIFactory2* factory = device->GetFactory();
	factory->CreateSwapChainForHwnd(device->GetD3D11Device(), window->GetHwnd(), &swapChainDesc, nullptr, nullptr, &mSwapChain);
	ObtainBuffer();
}

void DX11SwapChain::Present()
{
	mSwapChain->Present(1, 0); // Present with vsync
}

void DX11SwapChain::Resize()
{
	uvec2 newSize = mResize.load(std::memory_order_acquire);
	if (newSize.x == 0)
	{
		return;
	}
	mSize = newSize;
	mBackbuffer = nullptr;
	for (auto& vp : mViewports)
	{
		vp->Reset();
	}
	mSwapChain->ResizeBuffers(0, newSize.x, newSize.y, DXGI_FORMAT_UNKNOWN, 0);
	ObtainBuffer();
	if (mViewports.size() == 1)
	{
		mViewports[0]->Resize(mSize.x, mSize.y, GetBackbuffer());
	}
}

void DX11SwapChain::ObtainBuffer()
{
	ComPtr<ID3D11Texture2D> backBuffer;
	mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	DeviceTextureDesc desc;
	desc.Width = mSize.x;
	desc.Height = mSize.y;
	desc.Flags = TF_RenderTarget;// | TF_SRGB;
	mBackbuffer = std::make_shared<DX11Texture>(static_cast<DX11Device*>(mDevice)->GetDX11Ctx(),desc, backBuffer.Get());
	mResize.store(uvec2(0), std::memory_order_release);
}

}
