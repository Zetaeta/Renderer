#include "DX12Window.h"

namespace rnd::dx12
{

DX12Window::DX12Window(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers)
:wnd::Window(width,height,name)
{
	if (!DX12RHI::IsCreated())
	{
		DX12RHI::InitRHI();
	}
}

void DX12Window::Present()
{
	throw std::logic_error("The method or operation is not implemented.");
}

}
