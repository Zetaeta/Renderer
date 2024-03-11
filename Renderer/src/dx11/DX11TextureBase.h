#pragma once
#include "DX11Ctx.h"
#include "DeviceTexture.h"

enum ETextureFlags
{
	TF_NONE = 0,
	TF_DEPTH = 0x1,
	TF_SRGB = 0x2,
	TF_RENDER_TARGET = 0x3
};


struct DX11TextureDesc
{
	ETextureFlags flags;
	u32 width;
	u32 height;
	u32 numMips = 1;
	void const* data = nullptr;
};

class DX11TextureBase : public IDeviceTexture
{
public:
	DX11TextureBase(DX11Ctx& ctx, DX11TextureDesc const& desc)
		: m_Ctx(&ctx), m_Desc(desc)
	{
	}

protected:
	DX11Ctx* m_Ctx;
	DX11TextureDesc m_Desc;
};
