#pragma once

#include <memory>
namespace rnd
{
class IDeviceTexture;

using IDeviceTextureRef = std::shared_ptr<IDeviceTexture>;

class IDeviceMesh;

using DeviceMeshRef = RefPtr<IDeviceMesh>;

using VertAttDescHandle = s64;

}
