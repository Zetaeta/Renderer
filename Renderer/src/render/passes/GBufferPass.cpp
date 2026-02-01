#include "GBufferPass.h"
#include "render/dx11/DX11Renderer.h"
#include "render/ForwardRenderPass.h"
#include "render/ShadingCommon.h"
#include "imgui.h"

namespace rnd
{

GBufferPass::GBufferPass(RenderContext* rctx, Camera::Ref camera, RGRenderTargetRef rt1, RGRenderTargetRef rt2, RGDepthStencilRef ds)
	: SceneRenderPass(rctx, "GBuffer", camera, nullptr, nullptr, E_MT_OPAQUE | E_MT_MASKED), mAlbedoRT(rt1), mNormalRT(rt2), mDSV(ds)
{
}

void GBufferPass::Accept(DrawData const& dd)
{
	Draw(dd);
}

void GBufferPass::BeginRender()
{
	auto* PSPF = DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerFrame);
	PerFramePSData perFrameData;
	Zero(perFrameData);
	perFrameData.ambient = vec3(1);
	perFrameData.debugMode = Denum(mRCtx->Settings.ShadingDebugMode);
	perFrameData.debugGrayscaleExp = mRCtx->Settings.DebugGrayscaleExp;
	perFrameData.brdf = mRCtx->mBrdf;
	perFrameData.roughnessMod = roughnessMod;
	perFrameData.metalnessMod = metalnessMod;
	PSPF->Data().SetData(perFrameData);
	PSPF->Update();
	mLayer = EShadingLayer::GBuffer;
//	DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerFrame, sizeof(PerFramePSData));
	DeviceCtx()->SetBlendMode(BS_OPAQUE);
	DeviceCtx()->SetDepthStencilMode(EDepthMode::LessEqual, {EStencilMode::Disabled, 0});
	DeviceCtx()->ClearDepthStencil(mDSV.ResolvedDS, EDSClearMode::DEPTH, 1.f);
	DeviceCtx()->ClearRenderTarget(mNormalRT.ResolvedRT, col4(0));
	DeviceCtx()->ClearRenderTarget(mAlbedoRT.ResolvedRT, col4(0));
	Super::BeginRender();
	IRenderTarget::Ref rts[] = {mAlbedoRT.ResolvedRT, mNormalRT.ResolvedRT};
	DeviceCtx()->SetRTAndDS(rts, mDSV.ResolvedDS);

}

void GBufferPass::Build(RGBuilder& builder)
{
	mAlbedoRT.Resolve(builder);
	mNormalRT.Resolve(builder);
	mDSV.Resolve(builder);
}

void GBufferPass::AddControls()
{
	ImGui::DragFloat("Roughness modifier", &roughnessMod, 0.1f, -1.f, 1.f);
	ImGui::DragFloat("Metalness modifier", &metalnessMod, 0.1f, -1.f, 1.f);
}

}
