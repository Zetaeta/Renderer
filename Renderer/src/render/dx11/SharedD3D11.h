#pragma once
#include <d3d11.h>
#include <core/WinUtils.h>
#include <core/Types.h>
#include <core/Maths.h>
#include <core/Utils.h>
#include "render/DeviceTexture.h"


enum class ETextureFormat : u8;
namespace rnd
{
namespace dx11
{

class DX11Renderer;
class DX11RenderTarget;
class DX11Texture;
class DX11DepthStencil;
class DX11Cubemap;

using DX11TextureRef = std::shared_ptr<DX11Texture>;

template<typename ResourcePtr>
void SetResourceName(const ResourcePtr& resource, const String& str)
{
	if (!str.empty())
	{
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, (u32) str.size(), str.c_str());
	}
}

DXGI_FORMAT GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context = ETextureFormatContext::Resource);

}
}
