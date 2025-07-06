#pragma once

#include <memory>
#include "core/RefCounted.h"
using MaterialID = int;
#ifndef PROFILING
#define PROFILING 1
#endif

namespace rnd
{
class IDeviceTexture;
class IDeviceMesh;
class IRenderDevice;
class IRenderDeviceCtx;
class IConstantBuffer;
class GPUSyncPoint;

using DeviceTextureRef = std::shared_ptr<IDeviceTexture>;


using DeviceMeshRef = RefPtr<IDeviceMesh>;

using VertAttDescHandle = s64;


}

using ScreenObjectId = u32;

