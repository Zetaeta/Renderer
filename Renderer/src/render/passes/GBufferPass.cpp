#include "GBufferPass.h"
#include "render/dx11/DX11Renderer.h"

namespace rnd
{

GBufferPass::GBufferPass(RenderContext* rctx, Name const& name, Camera::Ref camera, IRenderTarget::Ref rt1, IRenderTarget::Ref rt2, IDepthStencil::Ref ds)
	:SceneRenderPass(rctx, name, camera, rt1, ds, E_MT_OPAQUE | E_MT_MASKED), mNormalRT(rt2)
{
}

void GBufferPass::Accept(SceneComponent const* sceneComp, MeshPart const* mesh)
{
	Draw({ mesh, sceneComp });
}

void GBufferPass::BeginRender()
{
	auto* PSPF = DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerFrame);
	dx11::PerFramePSData perFrameData;
	Zero(perFrameData);
	perFrameData.ambient = vec3(1);
	perFrameData.debugMode = Denum(mRCtx->ShadingDebugMode);
	perFrameData.debugGrayscaleExp = mRCtx->mDebugGrayscaleExp;
	perFrameData.brdf = mRCtx->mBrdf;
	PSPF->Data().SetData(perFrameData);
	PSPF->Update();
	mLayer = EShadingLayer::BASE;
//	DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerFrame, sizeof(PerFramePSData));
	DeviceCtx()->SetBlendMode(BS_OPAQUE);
	DeviceCtx()->SetDepthStencilMode(EDepthMode::LessEqual, {EStencilMode::Disabled, 0});
	Super::BeginRender();
	IRenderTarget::Ref rts[] = {mRenderTarget, mNormalRT};
	DeviceCtx()->SetRTAndDS(rts, mDepthStencil);

}

}
