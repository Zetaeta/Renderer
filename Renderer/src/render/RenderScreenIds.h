#pragma once

#include "SceneRenderPass.h"

namespace rnd
{
class RenderScreenIds : public SceneRenderPass
{
public:
	RenderScreenIds(RenderContext* ctx, Name passName, Camera::Ref camera, IRenderTarget::Ref renderTarget, IDepthStencil::Ref depthStencil = nullptr)
		: SceneRenderPass(ctx, passName, camera, renderTarget, depthStencil ? depthStencil : ctx->GetTempDepthStencilFor(renderTarget))
	{
		mMatOverride = DeviceCtx()->MatManager->GetDefaultMaterial(MAT_SCREEN_ID);
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
