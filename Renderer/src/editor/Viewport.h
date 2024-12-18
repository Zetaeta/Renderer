#pragma once
#include "scene/ScreenObject.h"
#include "core/Maths.h"
#include "core/Types.h"
#include <scene/Camera.h>
#include <render/DeviceTexture.h>

class Scene;

namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
}

class Viewport
{
public:
	Viewport(rnd::IRenderDeviceCtx* rdc, Scene* scene, Camera::Ref camera);
	Viewport(Scene* scene, rnd::IRenderDeviceCtx* rdc, ivec2 size);
	class ScreenObject* GetObjectAt(ivec2 pos);

	void Resize(u32 width, u32 height, rnd::IDeviceTexture::Ref backbuffer);

	void Reset();

	void Draw();

	rnd::RenderContext* GetRenderContext()
	{
		return mRCtx.get(); 
	}

	u32 GetWidth()
	{
		return mWidth;
	}

	u32 GetHeight()
	{
		return mHeight;
	}

	void DebugPixel(uint2 pixCoord);
	
public:
	u32 mWidth = 0;
	u32 mHeight = 0;
	OwningPtr<rnd::RenderContext> mRCtx = nullptr;
	Camera* mCamera = nullptr;
	Scene* mScene = nullptr;
	rnd::IRenderDeviceCtx* mDeviceCtx = nullptr;
};
 