#pragma once
#include "scene/ScreenObject.h"
#include "core/Maths.h"
#include "core/Types.h"
#include <scene/Camera.h>
#include <render/DeviceTexture.h>

class Scene;
namespace rnd { class IDeviceSurface; }
namespace rnd { class RendererScene; }

namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
}

class Viewport
{
public:
	Viewport(rnd::IRenderDeviceCtx* rdc, Scene* scene, Camera::Ref camera, rnd::IDeviceSurface* parentSurface = nullptr);
//	Viewport(Scene* scene, rnd::IRenderDeviceCtx* rdc, ivec2 size);
	~Viewport();
	class ScreenObject* GetObjectAt(ivec2 pos);

	void Resize(u32 width, u32 height, rnd::IDeviceTexture::Ref backbuffer);
	void UpdatePos(ivec2 pos)
	{
		mPos = pos;
	}

	uvec2 GetPos() const
	{
		return mPos;
	}

	void SetBackbuffer(rnd::IDeviceTexture::Ref backbuffer);

	void Reset();
	void SetScene(Scene* scene);

	void Draw(rnd::IRenderDeviceCtx& ctx);

	void OnClick(ivec2 position);

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

	Scene const* GetScene()
	{
		return mScene;
	}

	rnd::RendererScene* GetRenderScene() const
	{
		return mRScene;
	}

	ivec2 GetSize()
	{
		return {mWidth, mHeight};
	}


public:
	u32 mWidth = 0;
	u32 mHeight = 0;
	uvec2 mPos {0};
	OwningPtr<rnd::RenderContext> mRCtx = nullptr;
	Camera* mCamera = nullptr;
	Scene* mScene = nullptr;
	rnd::RendererScene* mRScene = nullptr;
	rnd::IRenderDeviceCtx* mDeviceCtx = nullptr;
	rnd::IDeviceSurface* mSurface = nullptr;
#if HIT_TESTING
//	rnd::DeviceTextureRef mScreenIdTex;
#endif
};
 