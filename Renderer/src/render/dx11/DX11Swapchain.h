#pragma once
#include "render/DeviceSurface.h"
#include "DX11Texture.h"

namespace rnd::dx11
{
class DX11Device;

class DX11SwapChain : public IDeviceSurface
{
public:
	DX11SwapChain(DX11Device* device, wnd::Window* window);
	void Present() override;

	void RequestResize(uvec2 newSize) override
	{
		mResize.store(newSize, std::memory_order_release);
	}

	DeviceTextureRef GetBackbuffer() override
	{
		return mBackbuffer;
	}

	bool IsResizeRequested() const
	{
		return mResize.load(std::memory_order_acquire).x != 0;
	}

	void Resize();

	void ObtainBuffer();

	std::atomic<uvec2> mResize;
	uvec2 mSize{0};
	DX11Texture::Ref mBackbuffer;
	ComPtr<IDXGISwapChain1> mSwapChain;
};

}
