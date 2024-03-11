#pragma once
#include <memory>

class IDeviceTexture
{
};

using DeviceTextureRef = std::shared_ptr<IDeviceTexture>;
//constexpr DeviceTextureRef INVALID_DEV_TEX = nullptr;
#define INVALID_DEV_TEX nullptr

