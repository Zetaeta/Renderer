#pragma once
#include "SceneRenderPass.h"

namespace rnd
{
class ShadowmapsPass : public SceneRenderPass
{
public:
	ShadowmapsPass(RenderContext* ctx, EMatType accepts = E_MT_OPAQUE | E_MT_MASKED)
		: SceneRenderPass(ctx, "Shadowmaps", nullptr, nullptr, nullptr, accepts, EShadingLayer::DEPTH, ERenderPassMode::DEFERRED)
	{
	}

	void OnCollectFinished() override;

	void RenderShadowmap(ELightType lightType, u32 lightIndex);
};

} // namespace rnd
