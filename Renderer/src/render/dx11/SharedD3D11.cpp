#include "SharedD3D11.h"
#include "render/DeviceTexture.h"


namespace rnd
{
namespace dx11
{

DXGI_FORMAT GetDxgiFormat(ETextureFormat textureFormat, EDxgiFormatContext context /*= EDxgiFormatContext::RESOURCE*/)
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
		case EDxgiFormatContext::Resource:
			return DXGI_FORMAT_R24G8_TYPELESS;
		case EDxgiFormatContext::SRV:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case EDxgiFormatContext::StencilSRV:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case EDxgiFormatContext::RenderTarget:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		}
		break;
	}
	case ETextureFormat::D32_Float:
	{
		switch (context)
		{
		case EDxgiFormatContext::Resource:
			return DXGI_FORMAT_R32_TYPELESS;
		case EDxgiFormatContext::DepthStencil:
			return DXGI_FORMAT_D32_FLOAT;
		case EDxgiFormatContext::SRV:
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
