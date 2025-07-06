#include "DeviceTexture.h"



u64 rnd::DeviceTextureDesc::GetSize() const
{
	u64 pixels = u64(Width) * Height;
	return pixels * GetPixelSize(Format);
}

u32 rnd::DeviceTextureDesc::GetRowPitch() const
{
	return Width * GetPixelSize(Format);
}

glm::u32 GetPixelSize(ETextureFormat format)
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
