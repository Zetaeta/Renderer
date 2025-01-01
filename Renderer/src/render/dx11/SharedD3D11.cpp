#include "SharedD3D11.h"
#include "render/DeviceTexture.h"


namespace rnd
{
namespace dx11
{

DXGI_FORMAT GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context /*= EDxgiFormatContext::RESOURCE*/)
{
	switch (textureFormat)
	{
	case ETextureFormat::RGBA8_Unorm:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ETextureFormat::RGBA8_Unorm_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ETextureFormat::R32_Uint:
		return DXGI_FORMAT_R32_UINT;
	case ETextureFormat::D24_Unorm_S8_Uint:
	{
		switch (context)
		{
		case ETextureFormatContext::Resource:
			return DXGI_FORMAT_R24G8_TYPELESS;
		case ETextureFormatContext::SRV:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case ETextureFormatContext::StencilSRV:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case ETextureFormatContext::RenderTarget:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		}
		break;
	}
	case ETextureFormat::D32_Float:
	{
		switch (context)
		{
		case ETextureFormatContext::Resource:
			return DXGI_FORMAT_R32_TYPELESS;
		case ETextureFormatContext::DepthStencil:
			return DXGI_FORMAT_D32_FLOAT;
		case ETextureFormatContext::SRV:
			return DXGI_FORMAT_R32_FLOAT;
		default:
			ZE_ASSERT(false);
		}
	}
	default:
		fprintf(stderr, "Invalid format\n");
	}
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

}
}
