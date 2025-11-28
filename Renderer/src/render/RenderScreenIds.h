#pragma once

#include "SceneRenderPass.h"
#include "MaterialManager.h"

namespace rnd
{
class RenderScreenIds : public SceneRenderPass
{
public:
	RenderScreenIds(RenderContext* ctx, HashString passName, Camera::Ref camera, IRenderTarget::Ref renderTarget, IDepthStencil::Ref depthStencil = nullptr)
		: SceneRenderPass(ctx, passName, camera, renderTarget, depthStencil ? depthStencil : ctx->GetTempDepthStencilFor(renderTarget))
	{
		mMatOverride = mRCtx->GetMaterialManager()->GetDefaultMaterial(MAT_SCREEN_ID);
	}

	virtual void BeginRender() override
	{
		SceneRenderPass::BeginRender();
		DeviceCtx()->ClearDepthStencil(mDepthStencil, EDSClearMode::DEPTH, 1.f);
		DeviceCtx()->ClearRenderTarget(mRenderTarget, {0,0,0,0});
		DeviceCtx()->SetDepthStencilMode(EDepthMode::Less);
		DeviceCtx()->SetBlendMode(EBlendState::NONE);
	}
};
} // namespace rnd
