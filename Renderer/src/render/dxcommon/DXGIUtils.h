#pragma once
#include <dxgiformat.h>
#include "core/CoreTypes.h"

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


namespace rnd::dx
{
DXGI_FORMAT GetFormatForType(TypeInfo const* type);

DXGI_FORMAT GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context = ETextureFormatContext::Resource);


}

