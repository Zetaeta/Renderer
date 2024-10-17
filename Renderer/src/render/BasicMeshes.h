#pragma once
#include "DeviceMesh.h"

namespace rnd
{

class IRenderDevice;

class BasicMeshFactory
{
public:
	BasicMeshFactory(IRenderDevice* device)
		: mDevice(device) {}

	DeviceMeshRef GetFullScreenTri();

private:
	RefPtr<IDeviceMesh> FullScreenTriangle;
	IRenderDevice*		mDevice;
};

} // namespace rnd
