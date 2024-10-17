#include "PostProcessPass.h"
#include "render/shaders/ShaderDeclarations.h"
#include "render/RenderContext.h"
#include "render/ShaderManager.h"

namespace rnd
{

 PostProcessPass::PostProcessPass(RenderContext* rCtx, PixelShader const* shader, IRenderTarget::Ref dest, ShaderResources&& resources)
	: RenderPass(rCtx), mShader(shader), mResources(std::move(resources)), mRenderTarget(dest)
{
	 mVertexShader = rCtx->GetShaderManager().GetCompiledShader<PostProcessVS>();
 }

void PostProcessPass::Execute(RenderContext& renderCtx)
{
	auto context = DeviceCtx();
	context->SetRTAndDS(mRenderTarget, nullptr);
//	context->ClearRenderTarget(mRenderTarget, )
	context->SetBlendMode(EBlendState::COL_OVERWRITE);
	context->SetViewport(mRenderTarget->Desc.Width, mRenderTarget->Desc.Height);
	context->SetVertexShader(mVertexShader);
	context->SetPixelShader(mShader);
	context->SetShaderResources(EShaderType::Pixel, mResources.SRVs);
	auto tri = Device()->BasicMeshes.GetFullScreenTri();
	ZE_REQUIRE(tri);
	context->SetVertexLayout(tri->GetVertexAttributes());
	context->DrawMesh(tri);
	context->SetShaderResources(EShaderType::Pixel, Vector<IDeviceTextureRef>(mResources.SRVs.size()));
}

}
