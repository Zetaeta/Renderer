#include "Viewport.h"
#include <render/RenderContext.h>


Viewport::Viewport(Scene* scene, rnd::IRenderDeviceCtx* rdc, ivec2 size)
{
}

Viewport::Viewport(rnd::IRenderDeviceCtx* rdc, Scene* scene, Camera::Ref camera)
	: mScene(scene), mCamera(camera), mDeviceCtx(rdc)
{
}

ScreenObject* Viewport::GetObjectAt(ivec2 pos)
{
	return nullptr;
}

void Viewport::Resize(u32 width, u32 height, IDeviceTexture::Ref backbuffer)
{
	mRCtx = MakeOwning<rnd::RenderContext>(mDeviceCtx, mCamera, backbuffer);
	mWidth = width;
	mHeight = height;
}

void Viewport::Reset()
{
	mWidth = mHeight = 0;
	mRCtx = nullptr;
}

void Viewport::Draw()
{
	mRCtx->RenderFrame(*mScene);
}
