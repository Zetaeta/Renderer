#include "RenderDevice.h"
#include "RendererScene.h"
#include "editor/Viewport.h"
#include "thread/ThreadUtils.h"

namespace rnd
{

IRenderDevice::IRenderDevice(IShaderCompiler* compiler)
: ShaderMgr(compiler), BasicMeshes(this), MatMgr(this), ResourceMgr(this)
{
	mGlobalIndex = gNumRenderDevices++;
	gRenderDevices[mGlobalIndex] = this;
}

IRenderDevice::~IRenderDevice()
{
	gRenderDevices[mGlobalIndex] = nullptr;
	MatMgr.Release();
}

void IRenderDevice::RegisterResource(IDeviceResource* resource)
{
	assert(IsInRenderThread());
	mResources.insert(resource);
}

void IRenderDevice::UnregisterResource(IDeviceResource* resource)
{
	assert(IsInRenderThread());
	mResources.erase(resource);
}

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
		vp->Draw(*GetPersistentCtx());
	}
	for (RendererScene* rScene : scenes)
	{
		rScene->EndFrame();
	}
	EndFrame();
}


u32 gNumRenderDevices = 0;
PerRenderDevice<IRenderDevice*> gRenderDevices;

}
