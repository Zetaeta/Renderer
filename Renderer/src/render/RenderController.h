#pragma once
#include "RndFwd.h"
#include "container/Vector.h"

namespace rnd
{
class RenderController
{
public:
	void BeginRenderFrame();
	void EndRenderFrame();

	void AddRenderBackend(IRenderDevice* rhi);
	void RemoveRenderBackend(IRenderDevice* rhi);
private:
	Vector<IRenderDevice*> mBackends;
};

extern RenderController GRenderController;
}
