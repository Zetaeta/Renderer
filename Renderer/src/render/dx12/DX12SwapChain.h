#pragma once
#include "render/DeviceSurface.h"
#include "SharedD3D12.h"
#include "dxgi1_4.h"

namespace wnd { class Window; }

namespace rnd::dx12
{
class DX12SwapChain : public IDeviceSurface
{
public:
	DX12SwapChain(wnd::Window* window, u32 numBuffers, uvec2 size);
	void Present() override;
	void RequestResize(uvec2 newSize) override;

	void ReleaseResourcesPreResize();
	void Resize();

//	void OnMoved(ivec2 newPos) override;

	void GetBuffers();

protected:	
	wnd::Window* mWindow;
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
