#pragma once
#include "render/RenderPass.h"
#include "render/Shader.h"
#include "render/DeviceTexture.h"
#include "render/RenderGraph.h"

namespace rnd
{

class PostProcessPass : public RenderPass
{
public:
	PostProcessPass(RenderContext* rCtx, PixelShader const* shader, RGRenderTargetRef dest, RGShaderResources&& resources, String&& name = "Postprocessing");

	virtual void Execute(RenderContext& renderCtx) override;

	void Build(RGBuilder& builder) override;

private:
	RefPtr<const PixelShader> mShader;
	RefPtr<const VertexShader> mVertexShader;
	RGShaderResources mResources;
	RGRenderTargetRef mRenderTarget;
	
};

}
