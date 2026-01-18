#pragma once

#include <memory>
#include "core/RefCounted.h"
#ifndef PROFILING
#define PROFILING 1
#endif

using MaterialID = int;
using PrimitiveId = u32;

constexpr PrimitiveId INVALID_PRIMITIVE = -1;
constexpr PrimitiveId InvalidPrimId()
{
	return (PrimitiveId) -1;
}

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


constexpr u32 gMaxRenderDevices = 4;
extern u32 gNumRenderDevices;
template<typename T>
using PerRenderDevice = std::array<T, gMaxRenderDevices>;

}

using ScreenObjectId = u32;

