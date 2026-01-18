#include "RenderController.h"
#include "RenderDevice.h"
#include "RenderingThread.h"
#include "execution"
#include "common/BufferedRenderInterface.h"

namespace rnd
{

RenderController GRenderController;

void RenderController::BeginRenderFrame()
{
	while (!BufferedRenderInterface::TryBeginFrame_RenderThread())
	{
		if (IsExitRequested())
		{
			return;
		}
		for (auto& rt : mRenderThreads)
		{
			rt->ProcessCommands();
		}
	}
	//for (SceneDataInterface* scene : SceneDataInterface::GetAll())
	//{
	//	scene->BeginFrame_RenderThread();
	//}
	std::for_each(std::execution::par, mRenderThreads.begin(), mRenderThreads.end(),
	[](auto& rt)
	{
		rt->RenderFrame();
	});
	//for (auto& rt : mRenderThreads)
	//{
	//	rt->RenderFrame();
	//}
}

void RenderController::EndRenderFrame()
{

	BufferedRenderInterface::EndFrame_RenderThread();
	//for (SceneDataInterface* scene : SceneDataInterface::GetAll())
	//{
	//	scene->EndFrame_RenderThread();
	//}
}

void RenderController::AddRenderBackend(IRenderDevice* rhi)
{
	mRenderThreads.push_back(MakeOwning<RenderingThread>(rhi));
}

void RenderController::RemoveRenderBackend(IRenderDevice* rhi)
{
	for (u32 i = 0; i < mRenderThreads.size(); ++i)
	{
		if (mRenderThreads[i]->GetDevice() == rhi)
		{
			RemoveSwapIndex(mRenderThreads, i);
			break;
		}
	}
}

void RenderController::EnqueueRenderCommand(std::function<void(IRenderDeviceCtx& ctx)>&& command, IRenderDevice* backend, const char* debugName)
{
	for (u32 i = 0; i < mRenderThreads.size(); ++i)
	{
		if (backend == mRenderThreads[i]->GetDevice() || backend == nullptr)
		{
			mRenderThreads[i]->AddCommand(command, debugName);
		}
	}
}

void RenderController::FlushRenderCommands(IRenderDevice* backend)
{
	for (u32 i = 0; i < mRenderThreads.size(); ++i)
	{
		if (backend == mRenderThreads[i]->GetDevice() || backend == nullptr)
		{
			mRenderThreads[i]->FlushCommands();
		}
	}
}

}
