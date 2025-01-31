#pragma once
#include "core/Types.h"
#include "RndFwd.h"

class Scene;
class Camera;
class Viewport;

namespace rnd
{
class IDeviceSurface
{
public:
	IDeviceSurface(IRenderDevice* device);
	virtual ~IDeviceSurface();

	virtual void Present() = 0;
	virtual void RequestResize(uvec2 newSize) = 0;
	virtual DeviceTextureRef GetBackbuffer() = 0;

	virtual void OnMoved(ivec2 newPos)
	{
		mPos = newPos;
	}

	Viewport* CreateFullscreenViewport(Scene* scene, Camera* camera);


protected:
	ivec2 mPos;
	uvec2 mSize;
	Vector<OwningPtr<Viewport>> mViewports;
	IRenderDevice* mDevice;
};

}
