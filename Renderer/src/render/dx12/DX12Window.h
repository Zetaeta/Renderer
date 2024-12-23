#pragma once

#include "core/Maths.h"
#include "platform/windows/WinApi.h"
#include "core/WinUtils.h"

struct ID3D12CommandQueue;
struct ID3D12Device6;
struct IDXGISwapChain;
using ID3D12Device_ = ID3D12Device6;

namespace rnd
{
enum ESwapchainBuffering
{
	DoubleBuffered = 2,
	TripleBuffered = 3
};

namespace dx12
{

class DX12Window
{
public:
	DX12Window(u32 width, u32 height, wchar_t const* name, ESwapchainBuffering numBuffers);
private:
	HWND mHwnd = (HWND) -1;
	u32 mWidth = 0;
	u32 mHeight = 0;
	
	void CreateDeviceAndCmdQueue();
	ComPtr<ID3D12CommandQueue> mCmdQueue;
	ComPtr<ID3D12Device_> mDevice; 
	ComPtr<IDXGISwapChain> mSwapChain; 
	ESwapchainBuffering mNumBuffers;
};

}
}
