#pragma once
#include "render/DeviceSurface.h"
#include "SharedD3D12.h"
#include "DX12Texture.h"
#include "dxgi1_4.h"

namespace wnd { class Window; }

namespace rnd::dx12
{
class DX12SwapChain : public IDeviceSurface
{
public:
	DX12SwapChain(wnd::Window* window, u32 numBuffers);
	void Present() override;
	void RequestResize(uvec2 newSize) override;

	void ReleaseResourcesPreResize();
	void Resize();

//	void OnMoved(ivec2 newPos) override;

	void GetBuffers();

	std::shared_ptr<DX12Texture> GetCurrentBuffer() const
	{
		return mSwapchainBufferTextures[mBufferIndex];
	}


	DeviceTextureRef GetBackbuffer() override
	{
		return GetCurrentBuffer();
	}

	bool IsResizeRequested() const
	{
		return mResizeWidth != 0;
	}

	DXGI_FORMAT GetRTVFormat() const;

	u32 GetNumBuffers() const { return mNumBuffers; }

protected:	
	uvec2 mSize;
	u32 mBufferIndex = 0;
	u32 mNumBuffers;
	ComPtr<IDXGISwapChain3> mSwapChain; 
	u32 mResizeWidth = 0;
	u32 mResizeHeight = 0;
	std::array<ComPtr<ID3D12Resource_>, MaxSwapchainBufferCount> mRenderTargets;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxSwapchainBufferCount> mRtvDescriptors;
	std::array<std::shared_ptr<DX12Texture>, MaxSwapchainBufferCount> mSwapchainBufferTextures;
};

}
