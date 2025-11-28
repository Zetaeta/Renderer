#pragma once

#include "SceneRenderPass.h"
#include <array>
#include <core/Utils.h>

namespace rnd
{

class ForwardRenderPass : public SceneRenderPass
{
	using Super = SceneRenderPass;
public:
	ForwardRenderPass(RenderContext* rctx, HashString const& name, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds);
	virtual void Accept(DrawData const& dd) override;
	virtual void OnCollectFinished() override;

	virtual void BeginRender() override;
	
};

} // namespace rnd
