#pragma once
#include "DeviceTexture.h"

namespace rnd
{

class IRenderDevice
{
public:
	virtual IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData = CubemapData{}) = 0;
	virtual IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData = nullptr) = 0;
};

}
