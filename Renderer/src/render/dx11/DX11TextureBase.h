#pragma once
#include "render/dx11/DX11Ctx.h"
#include "render/DeviceTexture.h"

class DX11TextureBase : public IDeviceTexture
{
public:
	DX11TextureBase(DX11Ctx& ctx, DeviceTextureDesc const& desc)
		:IDeviceTexture(desc), m_Ctx(&ctx)
	{
	}

protected:
	DX11Ctx* m_Ctx;
};
