#pragma once
#include "DeviceMesh.h"
#include "RndFwd.h"

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

class BasicTextureMgr
{
public:
	DeviceTextureRef GetBlackTexture(IRenderDevice* device);

	void Teardown();
private:
	DeviceTextureRef mBlack;
};


} // namespace rnd
