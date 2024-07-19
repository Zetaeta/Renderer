#include "Viewport.h"


Viewport::Viewport(Scene* scene, rnd::IRenderDeviceCtx* rdc, ivec2 size)
{
	mRCtx = MakeOwning<rnd::RenderContext>();
}

ScreenObject* Viewport::GetObjectAt(ivec2 pos)
{
}

void Viewport::Draw()
{
	mRCtx->RenderFrame();
}
