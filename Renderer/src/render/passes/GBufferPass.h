#pragma once
#include "render/SceneRenderPass.h"

namespace rnd
{

class GBufferPass : public SceneRenderPass
{
public:
	GBufferPass(RenderContext* rctx, Name const& name, Camera::Ref camera, IRenderTarget::Ref rt1, IRenderTarget::Ref rt2, IDepthStencil::Ref ds);
	using Super = SceneRenderPass;
	virtual void Accept(SceneComponent const* sceneComp, MeshPart const* mesh) override;
	virtual void BeginRender() override;
	IRenderTarget::Ref mNormalRT;
};
}
