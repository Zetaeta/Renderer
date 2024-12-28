#pragma once

#include <memory>
#include "core/RefCounted.h"
namespace rnd
{
class IDeviceTexture;

using IDeviceTextureRef = std::shared_ptr<IDeviceTexture>;

class IDeviceMesh;

using DeviceMeshRef = RefPtr<IDeviceMesh>;

using VertAttDescHandle = s64;

}
