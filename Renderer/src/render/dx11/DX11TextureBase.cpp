#include "DX11TextureBase.h"
#include "DX11Renderer.h"

 rnd::dx11::DX11TextureBase::DX11TextureBase(DX11Ctx& ctx, DeviceTextureDesc const& desc)
	: IDeviceTexture(ctx.m_Renderer, desc), m_Ctx(&ctx)
{
}
