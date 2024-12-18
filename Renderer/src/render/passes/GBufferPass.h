#pragma once
#include "render/SceneRenderPass.h"

namespace rnd
{

class GBufferPass : public SceneRenderPass
{
public:
	GBufferPass(RenderContext* rctx, Name const& name, Camera::Ref camera, RGRenderTargetRef rt1, RGRenderTargetRef rt2, RGDepthStencilRef ds);
	using Super = SceneRenderPass;
	virtual void Accept(SceneComponent const* sceneComp, MeshPart const* mesh) override;
	virtual void BeginRender() override;
	void Build(RGBuilder& builder) override;
	RGRenderTargetRef mAlbedoRT;
	RGRenderTargetRef mNormalRT;
	RGDepthStencilRef mDSV;
};
}
