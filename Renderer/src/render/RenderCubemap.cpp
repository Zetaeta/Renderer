#include "RenderCubemap.h"
#include "RenderDeviceCtx.h"
#include "RenderContext.h"
#include "dx11/DX11Ctx.h"
#include "dx11/DX11Renderer.h"
#include "shaders/ShaderDeclarations.h"
#include "VertexTypes.h"

namespace rnd
{

RenderCubemap::RenderCubemap(RenderContext* renderCtx, EFlatRenderMode mode, String DebugName /*= ""*/, IDeviceTextureCube* texture)
	:RenderPass(renderCtx, std::move(DebugName)), mCubemap(texture), mMode(mode)
{
	SetEnabled(texture != nullptr);
}

void RenderCubemap::Execute(IRenderDeviceCtx& deviceCtx)
{
	if (mCubemap == nullptr)
	{
		return;
	}
	auto context = &deviceCtx;
	deviceCtx.SetDepthStencilMode(mMode == EFlatRenderMode::BACK ? EDepthMode::Less : EDepthMode::Disabled);
	deviceCtx.SetBlendMode(EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE);
	std::shared_ptr<void> dummy;
	DeviceTextureRef	  sp(dummy, mCubemap);
	context->SetVertexShader(mRCtx->GetShader<CubemapVS>());
	context->SetPixelShader(mRCtx->GetShader<CubemapPS>());
	ResourceView srv {sp};
	context->SetShaderResources(EShaderType::Pixel, Single<ResourceView>(srv));
	context->SetConstantBuffers(EShaderType::Vertex, Single<IConstantBuffer* const>(context->GetConstantBuffer(ECBFrequency::VS_PerFrame)));
	context->SetVertexLayout(GetVertAttHdl<FlatVert>());
	context->DrawMesh(context->Device->BasicMeshes.GetFullScreenTri());
}

void RenderCubemap::SetCubemap(IDeviceTextureCube* cubemap)
{
	mCubemap = cubemap;
	SetEnabled(mCubemap != nullptr);
}

}
