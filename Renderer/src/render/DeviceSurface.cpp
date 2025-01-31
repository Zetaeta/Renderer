#include "DeviceSurface.h"
#include "editor/Viewport.h"

namespace rnd
{

IDeviceSurface::IDeviceSurface(IRenderDevice* device)
	: mDevice(device)
{
}

IDeviceSurface::~IDeviceSurface()
{
}

Viewport* IDeviceSurface::CreateFullscreenViewport(Scene* scene, Camera* camera)
{
	ZE_ASSERT(mViewports.empty());
	Viewport* vp = mViewports.emplace_back(MakeOwning<Viewport>(mDevice->GetPersistentCtx(), scene, camera)).get();
	vp->Resize(mSize.x, mSize.y, GetBackbuffer());
	return vp;
}

}
