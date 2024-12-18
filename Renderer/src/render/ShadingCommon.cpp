#include "ShadingCommon.h"
#include "core/Utils.h"
#include "RenderContext.h"
#include "scene/Scene.h"

namespace rnd
{


void SetupShadingLayer(RenderContext const* renderContext, EShadingLayer layer, u32 index)
{
	IRenderDeviceCtx* deviceCtx = renderContext->DeviceCtx();
	Scene const& scene = renderContext->GetScene();
	auto& PSPF = *deviceCtx->GetConstantBuffer(ECBFrequency::PS_PerFrame); //static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerFramePSCB();

	if (layer == EShadingLayer::BASE)
	{
		PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(scene.m_AmbientLight);
		perFrameData.debugMode = Denum(renderContext->Settings.ShadingDebugMode);
		perFrameData.debugGrayscaleExp = renderContext->Settings.DebugGrayscaleExp;
		perFrameData.brdf = renderContext->mBrdf;
		PSPF.WriteData(perFrameData);
		deviceCtx->SetBlendMode(BS_OPAQUE);
		return;
	}
	if (!RCHECK(layer < EShadingLayer::SHADING_COUNT))
	{
		return;
	}
	

	deviceCtx->SetBlendMode(BS_OPAQUE_LAYER);
	deviceCtx->SetDepthMode(EDepthMode::Equal | EDepthMode::NoWrite);
	LightRenderData const& lrd = renderContext->GetLightData(GetLightFromLayer(layer), index);
	switch (layer)
	{
	case EShadingLayer::DIRLIGHT:
	{
		auto const& light = scene.m_PointLights[index];
		PFPSDirLight perFrameData;
		Zero(perFrameData);
		perFrameData.directionalCol = vec3(scene.m_DirLights[index].colour);
		perFrameData.directionalDir = scene.m_DirLights[index].dir;
		perFrameData.world2Light = lrd.mCamera->GetProjWorld();;
		perFrameData.debugMode = Denum(renderContext->Settings.ShadingDebugMode);
		perFrameData.debugGrayscaleExp = renderContext->Settings.DebugGrayscaleExp;
		perFrameData.brdf = renderContext->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	case EShadingLayer::SPOTLIGHT:
	{
		auto const& light = scene.m_SpotLights[index];
		PFPSSpotLight perFrameData;
		Zero(perFrameData);
		perFrameData.spotLightCol = light.colour;
		perFrameData.spotLightDir = light.dir;
		perFrameData.spotLightPos = light.pos;
		perFrameData.spotLightTan = light.tangle;
		perFrameData.spotLightFalloff = light.falloff;
		perFrameData.world2Light = lrd.mCamera->GetProjWorld();
		perFrameData.debugMode = Denum(renderContext->Settings.ShadingDebugMode);
		perFrameData.debugGrayscaleExp = renderContext->Settings.DebugGrayscaleExp;
		perFrameData.brdf = renderContext->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	case EShadingLayer::POINTLIGHT:
	{
		auto const& light = scene.m_PointLights[index];
		PFPSPointLight perFrameData;
		Zero(perFrameData);
		perFrameData.pointLightPos = light.pos;
		perFrameData.pointLightCol = light.colour;
		perFrameData.pointLightRad = light.radius;
		perFrameData.pointLightFalloff = light.falloff;
		perFrameData.world2Light = lrd.mCamera->WorldToCamera();
		perFrameData.debugMode = Denum(renderContext->Settings.ShadingDebugMode);
		perFrameData.debugGrayscaleExp = renderContext->Settings.DebugGrayscaleExp;
		perFrameData.brdf = renderContext->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	default:
		break;
	}
	deviceCtx->TextureManager->SetTexture(E_TS_SHADOWMAP, lrd.mShadowMap);

}

}
