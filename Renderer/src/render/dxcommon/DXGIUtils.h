#pragma once
#include "core/BaseDefines.h"
#include <dxgiformat.h>
#include "core/CoreTypes.h"
#include <d3dcommon.h>

class TypeInfo;

enum class ETextureFormatContext : u8
{
	Resource,
	SRV,
	UAV,
	RenderTarget,
	DepthStencil = RenderTarget,
	StencilSRV
};
enum class ETextureFormat: u8;


namespace rnd
{
enum class ESystemValue : u32;
}

namespace rnd::dx
{
DXGI_FORMAT GetFormatForType(TypeInfo const* type);

DXGI_FORMAT GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context = ETextureFormatContext::Resource);

ESystemValue TranslateSV(D3D_NAME d3dSystemVal);

}

