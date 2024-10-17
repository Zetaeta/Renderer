#include "RenderCubemap.h"
#include "RenderDeviceCtx.h"
#include "RenderContext.h"
#include "dx11/DX11Ctx.h"
#include "dx11/DX11Renderer.h"

namespace rnd
{

RenderCubemap::RenderCubemap(RenderContext* renderCtx, EFlatRenderMode mode, String DebugName /*= ""*/, IDeviceTextureCube* texture)
	:RenderPass(renderCtx), mCubemap(texture), mMode(mode)
{
	SetEnabled(texture != nullptr);
}

void RenderCubemap::Execute(RenderContext& renderCtx)
{
	renderCtx.DeviceCtx()->SetDepthMode(mMode == EFlatRenderMode::BACK ? EDepthMode::LESS : EDepthMode::DISABLED);
	renderCtx.DeviceCtx()->SetBlendMode(EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE);
	renderCtx.DeviceCtx()->DrawCubemap(mCubemap);
}

void RenderCubemap::SetCubemap(IDeviceTextureCube* cubemap)
{
	mCubemap = cubemap;
	SetEnabled(mCubemap != nullptr);
}

}
