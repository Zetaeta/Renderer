#include "RenderController.h"
#include "RenderDevice.h"

namespace rnd
{

RenderController GRenderController;

void RenderController::BeginRenderFrame()
{
	for (IRenderDevice* rhi : mBackends)
	{
		rhi->RenderFrame();
	}
}

void RenderController::EndRenderFrame()
{

}

void RenderController::AddRenderBackend(IRenderDevice* rhi)
{
	mBackends.push_back(rhi);
}

void RenderController::RemoveRenderBackend(IRenderDevice* rhi)
{
	Remove(mBackends, rhi);
}

}
