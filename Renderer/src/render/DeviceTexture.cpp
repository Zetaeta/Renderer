#include "DeviceTexture.h"



u64 rnd::DeviceTextureDesc::GetSize() const
{
	u64 pixels = u64(Width) * Height;
	switch (Format)
	{
	case ETextureFormat::RGBA8_Unorm:
	case ETextureFormat::RGBA8_Unorm_SRGB:
	case ETextureFormat::D24_Unorm_S8_Uint:
	case ETextureFormat::D32_Float:
	case ETextureFormat::R32_Uint:
		return 4 * pixels;
	case ETextureFormat::R16_Float:
		return 2 * pixels;
	default:
		ZE_ASSERT(false);
		return 0;
	}
}

u32 rnd::DeviceTextureDesc::GetRowPitch() const
{
	switch (Format)
	{
	case ETextureFormat::RGBA8_Unorm:
	case ETextureFormat::RGBA8_Unorm_SRGB:
	case ETextureFormat::D24_Unorm_S8_Uint:
	case ETextureFormat::D32_Float:
	case ETextureFormat::R32_Uint:
		return 4 * Width;
	case ETextureFormat::R16_Float:
		return 2 * Width;
	default:
		ZE_ASSERT(false);
		return 0;
	}
}
