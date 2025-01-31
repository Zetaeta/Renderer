#include "ForwardRenderPass.h"
#include "dx11/DX11Renderer.h"
#include "scene/Lights.h"
#include "ShadingCommon.h"

namespace rnd
{

ForwardRenderPass::ForwardRenderPass(RenderContext* rctx, Name const& name, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds)
	: SceneRenderPass(rctx, name, camera, rt, ds)
{
}

void ForwardRenderPass::Accept(DrawData const& dd)
{
	mBuffer.emplace_back(dd);
	if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::BASE)])
	{
		Draw(dd);
	}
}

void ForwardRenderPass::OnCollectFinished()
{
	for (u8 l = 0; l < Denum(ELightType::COUNT); ++l)
	{
		ELightType lightType = EnumCast<ELightType>(l);
		EShadingLayer layer = GetLightLayer(lightType);
		if (//Settings.LayersEnabled[Denum(layer)] &&
			mRCtx->Settings.LayersEnabled[Denum(layer)])
		{
			for (int i = 0; i < mRCtx->GetLightData(lightType).size(); ++i)
			{
				mLayer = layer;
				SetupShadingLayer(mRCtx, *DeviceCtx(), layer, i);
				ProcessBuffer();
			}
		}
	}
	mBuffer.clear();
}

void ForwardRenderPass::BeginRender()
{
	mLayer = EShadingLayer::BASE;
	SetupShadingLayer(mRCtx, *DeviceCtx(), EShadingLayer::BASE, -1);
	DeviceCtx()->SetDepthStencilMode(EDepthMode::Less, {EStencilMode::Disabled, 0});
	Super::BeginRender();
}

}
