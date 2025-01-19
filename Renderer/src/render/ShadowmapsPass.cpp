#include "ShadowmapsPass.h"
#include "dx11/DX11Renderer.h"



namespace rnd
{

void ShadowmapsPass::OnCollectFinished()
{
	mLayer = EShadingLayer::Depth;
	DeviceCtx()->SetDepthStencilMode(EDepthMode::Less);
	for (ELightType lightType = ELightType::START; lightType < ELightType::COUNT; ++lightType)
	{
		auto const& lrd = mRCtx->GetLightData(lightType);
		for (u32 i = 0; i < lrd.size(); ++i)
		{
			RenderShadowmap(lightType, i);
		}
	}
	mBuffer.clear();
}

void ShadowmapsPass::RenderShadowmap(ELightType lightType, u32 lightIndex)
{
	auto const& lrd = mRCtx->GetLightData(lightType, lightIndex);
	if (lrd.mShadowMap)
	{
		mDepthStencil = lrd.mShadowMap->GetDS();
		DeviceCtx()->ClearDepthStencil(mDepthStencil, EDSClearMode::DEPTH, 1.f);
		mViewCam = lrd.GetCamera();
		if (lightType == ELightType::POINT_LIGHT)
		{
			mMatOverride = mRCtx->GetMaterialManager()->GetDefaultMaterial(MAT_POINT_SHADOW_DEPTH);
		}
		RenderBuffer();
	}
	mMatOverride = nullptr;
}

} // namespace rnd
