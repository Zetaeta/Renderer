#pragma once

#include "DX12RHI.h"
#include "platform/windows/Window.h"
#include "render/DeviceSurface.h"
namespace rnd
{
namespace dx12
{

class DX12Window : public wnd::Window, public IDeviceSurface
{
	DX12Window(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers);

	void Present() override;

};

}
}
