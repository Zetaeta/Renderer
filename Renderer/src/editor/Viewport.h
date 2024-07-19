#pragma once
#include "scene/ScreenObject.h"
#include "core/Maths.h"
#include "core/Types.h"

struct Scene;

namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
}

class Viewport
{
	Viewport(Scene* scene, rnd::IRenderDeviceCtx* rdc, ivec2 size);
	ScreenObject* GetObjectAt(ivec2 pos);

	void Draw();
	
	OwningPtr<rnd::RenderContext> mRCtx = nullptr;
};
