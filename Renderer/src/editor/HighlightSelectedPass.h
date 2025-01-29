#pragma once
#include "render/RenderPass.h"
#include <scene/SceneComponent.h>

namespace rnd
{

class HighlightSelectedPass : public RenderPass
{
public:
	HighlightSelectedPass(RenderContext* rCtx)
		: RenderPass(rCtx) {}
	void Execute(IRenderDeviceCtx& deviceCtx) override;
	void DrawPrimitive(const PrimitiveComponent* primComp);
};

}
