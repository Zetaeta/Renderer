#include "DeviceTexture.h"

namespace rnd
{

u64 DeviceTextureDesc::GetSize() const
{
	u64 pixels = u64(Width) * Height;
	return pixels * GetPixelSize(Format);
}

u32 DeviceTextureDesc::GetRowPitch() const
{
	return Width * GetPixelSize(Format);
}

u32 GetNumChannels(ETextureFormat format)
{
	switch (format)
	{
		case ETextureFormat::RGBA8_Unorm:
		case ETextureFormat::RGBA8_Unorm_SRGB:
			return 4;
		case ETextureFormat::D24_Unorm_S8_Uint:
			return 2;
		case ETextureFormat::D32_Float:
		case ETextureFormat::R32_Uint:
		case ETextureFormat::R16_Float:
		case ETextureFormat::R32_Float:
			return 1;
		default:
			assert(false);
			return 4;
	}
}

u32 GetPixelSize(ETextureFormat format)
{
	switch (format)
	{
	case ETextureFormat::RGBA8_Unorm:
	case ETextureFormat::RGBA8_Unorm_SRGB:
	case ETextureFormat::D24_Unorm_S8_Uint:
	case ETextureFormat::D32_Float:
	case ETextureFormat::R32_Float:
	case ETextureFormat::R32_Uint:
		return 4;
	case ETextureFormat::R16_Float:
		return 2;
	default:
		ZE_ASSERT(false);
		return 4;
	}
}

}
