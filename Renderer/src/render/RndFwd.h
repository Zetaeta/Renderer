#pragma once

#include <memory>
#include "core/RefCounted.h"
using MaterialID = int;

namespace rnd
{
class IDeviceTexture;

using DeviceTextureRef = std::shared_ptr<IDeviceTexture>;

class IDeviceMesh;

using DeviceMeshRef = RefPtr<IDeviceMesh>;

using VertAttDescHandle = s64;

}
