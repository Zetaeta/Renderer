#include "DeviceSurface.h"
#include "editor/Viewport.h"
#include "platform/windows/Window.h"
#include "RenderDevice.h"

namespace rnd
{

IDeviceSurface::IDeviceSurface(IRenderDevice* device, wnd::Window* window)
	: mDevice(device), mWindow(window)
{
	RECT rect;
	if (GetWindowRect(window->GetHwnd(), &rect))
	{
		mPos = {rect.left, rect.top};
		mSize = { NumCast<u32>(rect.right - rect.left), NumCast<u32>(rect.bottom - rect.top) };
	}
	window->SetSurface(this);
}

IDeviceSurface::~IDeviceSurface()
{
}

void IDeviceSurface::OnMoved(ivec2 newPos)
{
	mPos = newPos;
	if (mViewports.size() == 1)
	{
		mViewports[0]->UpdatePos(mPos);
	}
}

Viewport* IDeviceSurface::CreateFullscreenViewport(Scene* scene, Camera* camera)
{
	ZE_ASSERT(mViewports.empty());
	Viewport* vp = mViewports.emplace_back(MakeOwning<Viewport>(mDevice->GetPersistentCtx(), scene, camera, this)).get();
	vp->Resize(mSize.x, mSize.y, GetBackbuffer());
	return vp;
}

}
