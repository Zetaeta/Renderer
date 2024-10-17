#pragma once
#include "render/RenderPass.h"
#include "render/Shader.h"
#include "render/DeviceTexture.h"

namespace rnd
{

class PostProcessPass : public RenderPass
{
public:
	PostProcessPass(RenderContext* rCtx, PixelShader const* shader, IRenderTarget::Ref dest, ShaderResources&& resources);

	virtual void Execute(RenderContext& renderCtx) override;

private:
	RefPtr<const PixelShader> mShader;
	RefPtr<const VertexShader> mVertexShader;
	ShaderResources mResources;
	IRenderTarget::Ref mRenderTarget;
	
};

}
