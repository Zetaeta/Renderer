#include "ForwardRenderPass.h"
#include "dx11/DX11Renderer.h"
#include "scene/Lights.h"

namespace rnd
{

ForwardRenderPass::ForwardRenderPass(RenderContext* rctx, Name const& name, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds)
	: SceneRenderPass(rctx, name, camera, rt, ds)
{
}

void ForwardRenderPass::Accept(SceneComponent const* sceneComp, MeshPart const* mesh)
{
	DrawData const& dd = mBuffer.emplace_back(mesh, sceneComp);
	if (mRCtx->mLayersEnabled[Denum(EShadingLayer::BASE)])
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
		if (//mLayersEnabled[Denum(layer)] &&
			mRCtx->mLayersEnabled[Denum(layer)])
		{
			for (int i = 0; i < mRCtx->GetLightData(lightType).size(); ++i)
			{
				SetupLayer(layer, i);
				ProcessBuffer();
			}
		}
	}
	mBuffer.clear();
}

void ForwardRenderPass::BeginRender()
{
	mLayer = EShadingLayer::BASE;
	SetupLayer(EShadingLayer::BASE, -1);
	DeviceCtx()->SetDepthStencilMode(EDepthMode::Less, {EStencilMode::Disabled, 0});
	Super::BeginRender();
}

void ForwardRenderPass::SetupLayer(EShadingLayer layer, u32 index)
{
	mLayer = layer;
	auto& PSPF = static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerFramePSCB();

	using namespace dx11;
	if (layer == EShadingLayer::BASE)
	{
		PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(mScene->m_AmbientLight);
		perFrameData.debugMode = Denum(mRCtx->ShadingDebugMode);
		perFrameData.debugGrayscaleExp = mRCtx->mDebugGrayscaleExp;
		perFrameData.brdf = mRCtx->mBrdf;
		PSPF.WriteData(perFrameData);
		DeviceCtx()->SetBlendMode(BS_OPAQUE);
		return;
	}
	if (!RCHECK(layer < EShadingLayer::SHADING_COUNT))
	{
		return;
	}
	

	DeviceCtx()->SetBlendMode(BS_OPAQUE_LAYER);
	DeviceCtx()->SetDepthMode(EDepthMode::Equal | EDepthMode::NoWrite);
	LightRenderData const& lrd = mRCtx->GetLightData(GetLightFromLayer(layer), index);
	switch (layer)
	{
	case EShadingLayer::DIRLIGHT:
	{
		auto const& light = mScene->m_PointLights[index];
		PFPSDirLight perFrameData;
		Zero(perFrameData);
		perFrameData.directionalCol = vec3(mScene->m_DirLights[index].colour);
		perFrameData.directionalDir = mScene->m_DirLights[index].dir;
		perFrameData.world2Light = lrd.mCamera->GetProjWorld();;
		perFrameData.debugMode = Denum(mRCtx->ShadingDebugMode);
		perFrameData.debugGrayscaleExp = mRCtx->mDebugGrayscaleExp;
		perFrameData.brdf = mRCtx->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	case EShadingLayer::SPOTLIGHT:
	{
		auto const& light = mScene->m_SpotLights[index];
		PFPSSpotLight perFrameData;
		Zero(perFrameData);
		perFrameData.spotLightCol = light.colour;
		perFrameData.spotLightDir = light.dir;
		perFrameData.spotLightPos = light.pos;
		perFrameData.spotLightTan = light.tangle;
		perFrameData.spotLightFalloff = light.falloff;
		perFrameData.world2Light = lrd.mCamera->GetProjWorld();
		perFrameData.debugMode = Denum(mRCtx->ShadingDebugMode);
		perFrameData.debugGrayscaleExp = mRCtx->mDebugGrayscaleExp;
		perFrameData.brdf = mRCtx->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	case EShadingLayer::POINTLIGHT:
	{
		auto const& light = mScene->m_PointLights[index];
		PFPSPointLight perFrameData;
		Zero(perFrameData);
		perFrameData.pointLightPos = light.pos;
		perFrameData.pointLightCol = light.colour;
		perFrameData.pointLightRad = light.radius;
		perFrameData.pointLightFalloff = light.falloff;
		perFrameData.world2Light = lrd.mCamera->WorldToCamera();
		perFrameData.debugMode = Denum(mRCtx->ShadingDebugMode);
		perFrameData.debugGrayscaleExp = mRCtx->mDebugGrayscaleExp;
		perFrameData.brdf = mRCtx->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	default:
		break;
	}
	DeviceCtx()->TextureManager->SetTexture(E_TS_SHADOWMAP, lrd.mShadowMap);
}

}
