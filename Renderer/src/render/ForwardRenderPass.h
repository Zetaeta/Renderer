#pragma once

#include "SceneRenderPass.h"
#include <array>
#include "dx11/DX11Material.h"
#include <core/Utils.h>

namespace rnd
{
class ForwardRenderPass : public SceneRenderPass
{
	using Super = SceneRenderPass;
public:
	ForwardRenderPass(RenderContext* rctx, Name const& name, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds);
	std::array<bool, Denum(EShadingLayer::SHADING_COUNT)> mLayersEnabled;
	virtual void Accept(SceneComponent const* sceneComp, MeshPart const* mesh) override;
	virtual void OnCollectFinished() override;

	virtual void BeginRender() override;
	
	void SetupLayer(EShadingLayer lightType, u32 lightIdx);
};

} // namespace rnd
