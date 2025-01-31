#pragma once
#include "core/Types.h"
#include "RndFwd.h"

class Scene;
class Camera;
class Viewport;
namespace wnd { class Window; }

namespace rnd
{
class IDeviceSurface
{
public:
	IDeviceSurface(IRenderDevice* device, wnd::Window* window);
	virtual ~IDeviceSurface();

	virtual void Present() = 0;
	virtual void RequestResize(uvec2 newSize) = 0;
	virtual DeviceTextureRef GetBackbuffer() = 0;

	virtual void OnMoved(ivec2 newPos);

	Viewport* CreateFullscreenViewport(Scene* scene, Camera* camera);


protected:
	ivec2 mPos{0};
	uvec2 mSize{0};
	wnd::Window* mWindow;
	Vector<OwningPtr<Viewport>> mViewports;
	IRenderDevice* mDevice;
};

}
