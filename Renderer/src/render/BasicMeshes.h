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
	DeviceMeshRef GetFullscreenSquare();
	void Teardown() {FullScreenTriangle = nullptr; mSquare = nullptr;}

private:
	RefPtr<IDeviceMesh> FullScreenTriangle;
	RefPtr<IDeviceMesh> mSquare;
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
