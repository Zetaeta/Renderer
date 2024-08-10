#pragma once
#include <d3d11.h>
#include <core/WinUtils.h>
#include <core/Types.h>
#include <core/Maths.h>
#include <core/Utils.h>

class DX11Renderer;
class DX11RenderTarget;
class DX11Texture;
class DX11DepthStencil;
class DX11Cubemap;

namespace rnd
{
namespace dx11
{

template<typename ResourcePtr>
void SetResourceName(const ResourcePtr& resource, const String& str)
{
	if (!str.empty())
	{
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, (u32) str.size(), str.c_str());
	}
}

}
}
