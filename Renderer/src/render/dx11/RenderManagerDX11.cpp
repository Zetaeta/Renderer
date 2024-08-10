#include "RenderManagerDX11.h"

rnd::IRenderDeviceCtx* rnd::dx11::RenderManagerDX11::DeviceCtx()
{
	return Renderer();
}

rnd::IRenderDevice* rnd::dx11::RenderManagerDX11::Device()
{
	return Renderer();
}
