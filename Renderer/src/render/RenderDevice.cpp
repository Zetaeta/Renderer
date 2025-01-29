#include "RenderDevice.h"
#include "RendererScene.h"
#include "editor/Viewport.h"

namespace rnd
{

void IRenderDevice::BeginFrame()
{
	ResourceMgr.ProcessTextureCreations();
}

void IRenderDevice::RenderFrame()
{
	BeginFrame();
	SmallVector<RendererScene*, 8> scenes;
	for (Viewport* vp : mViewports)
	{
		RendererScene* rScene = vp->GetRenderScene();
		if (Find(scenes, rScene) == nullptr)
		{
			scenes.push_back(rScene);
			rScene->BeginFrame();
		}
		vp->Draw();
	}
	for (RendererScene* rScene : scenes)
	{
		rScene->EndFrame();
	}
	EndFrame();
}

}
