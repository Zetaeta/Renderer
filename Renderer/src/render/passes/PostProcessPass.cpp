#include "PostProcessPass.h"
#include "render/shaders/ShaderDeclarations.h"
#include "render/RenderContext.h"
#include "render/ShaderManager.h"

namespace rnd
{

 PostProcessPass::PostProcessPass(RenderContext* rCtx, PixelShader const* shader, RGRenderTargetRef dest, RGShaderResources&& resources, String&& name)
	: RenderPass(rCtx, std::move(name)), mShader(shader), mResources(std::move(resources)), mRenderTarget(dest)
{
	 mVertexShader = rCtx->GetShaderManager().GetCompiledShader<PostProcessVS>();
 }

void PostProcessPass::Execute(RenderContext& renderCtx)
{
	auto context = DeviceCtx();
	context->SetRTAndDS(mRenderTarget.ResolvedRT, nullptr);
//	context->ClearRenderTarget(mRenderTarget, )
	context->SetBlendMode(EBlendState::COL_OVERWRITE);
	context->SetViewport(mRenderTarget.ResolvedRT->Desc.Width, mRenderTarget.ResolvedRT->Desc.Height);
	context->SetVertexShader(mVertexShader);
	context->SetPixelShader(mShader);
	context->SetShaderResources(EShaderType::Pixel, mResources.ResolvedViews);
	auto tri = Device()->BasicMeshes.GetFullScreenTri();
	ZE_REQUIRE(tri);
	context->SetVertexLayout(tri->GetVertexAttributes());
	context->DrawMesh(tri);
	context->ClearResourceBindings();
}

void PostProcessPass::Build(RGBuilder& builder)
{
	mResources.Resolve(builder);
	mRenderTarget.Resolve(builder);
}

}
