#include "Viewport.h"
#include <render/RenderContext.h>
#include "render/RendererScene.h"


Viewport::Viewport(rnd::IRenderDeviceCtx* rdc, Scene* scene, Camera::Ref camera)
	: mScene(scene), mRScene(rnd::RendererScene::Get(*scene, rdc)), mCamera(camera), mDeviceCtx(rdc)
{
	rdc->Device->AddViewport(this);
}

Viewport::~Viewport()
{

	mDeviceCtx->Device->RemoveViewport(this);
}

ScreenObject* Viewport::GetObjectAt(ivec2 pos)
{
	return nullptr;
}

void Viewport::Resize(u32 width, u32 height, rnd::IDeviceTexture::Ref backbuffer)
{
	rnd::RenderSettings settings;
	if (mRCtx)
	{
		settings = mRCtx->Settings;
	}
	mRCtx = MakeOwning<rnd::RenderContext>(mDeviceCtx, mRScene, mCamera, backbuffer, settings);
	mWidth = width;
	mHeight = height;
}

void Viewport::Reset()
{
	mWidth = mHeight = 0;
	mRCtx = nullptr;
}

void Viewport::SetScene(Scene* scene)
{
	mRScene = rnd::RendererScene::Get(*scene, mDeviceCtx);
}

void Viewport::Draw()
{
	mRCtx->RenderFrame();
}

void Viewport::DebugPixel(uint2 pixCoord)
{
	mRCtx->DebugPixel(pixCoord);
}
