#pragma once
#include "render/SceneRenderPass.h"

namespace rnd
{

class GBufferPass : public SceneRenderPass
{
public:
	GBufferPass(RenderContext* rctx, Camera::Ref camera, RGRenderTargetRef rt1, RGRenderTargetRef rt2, RGDepthStencilRef ds);
	using Super = SceneRenderPass;
	virtual void Accept(DrawData const& dd) override;
	virtual void BeginRender() override;
	void Build(RGBuilder& builder) override;
	void AddControls() override;
	RGRenderTargetRef mAlbedoRT;
	RGRenderTargetRef mNormalRT;
	RGDepthStencilRef mDSV;

	float roughnessMod = 0;
	float metalnessMod = 0;
};
}
