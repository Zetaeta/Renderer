#include "PostProcessPass.h"
#include "render/shaders/ShaderDeclarations.h"
#include "render/RenderContext.h"
#include "render/ShaderManager.h"

namespace rnd
{

 PostProcessPass::PostProcessPass(RenderContext* rCtx, PixelShader const* shader, RGRenderTargetRef dest, RGShaderResources&& resources, String&& name)
	: RenderPass(rCtx, std::move(name)), mShader(shader), mResources(std::move(resources)), mRenderTarget(dest)
{
	 mVertexShader = rCtx->GetShader<PostProcessVS>();
 }

void PostProcessPass::Execute(IRenderDeviceCtx& deviceCtx)
{
	auto context = &deviceCtx;
	context->SetRTAndDS(mRenderTarget.ResolvedRT, nullptr);
//	context->ClearRenderTarget(mRenderTarget, )
	context->SetBlendMode(EBlendState::COL_OVERWRITE);
	context->SetViewport(mRenderTarget.ResolvedRT->Desc.Width, mRenderTarget.ResolvedRT->Desc.Height);
	context->SetPixelShader(mShader);
	context->SetShaderResources(EShaderType::Pixel, mResources.ResolvedViews);
	auto tri = Device()->BasicMeshes.GetFullScreenTri();
	ZE_REQUIRE(tri);
	context->SetVertexLayout(-1);
	context->SetVertexShader(nullptr);
	context->SetVertexLayout(tri->GetVertexAttributes());
	context->SetVertexShader(mVertexShader);
	context->DrawMesh(tri);
	context->ClearResourceBindings();
}

void PostProcessPass::Build(RGBuilder& builder)
{
	mResources.Resolve(builder);
	mRenderTarget.Resolve(builder);
}

}
