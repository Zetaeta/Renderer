#include "ShadingCommon.h"
#include "core/Utils.h"
#include "RenderContext.h"
#include "scene/Scene.h"
#include "RendererScene.h"

namespace rnd
{


void SetupShadingLayer(RenderContext* renderContext, IRenderDeviceCtx& ctx, EShadingLayer layer, u32 index)
{
	IRenderDeviceCtx* deviceCtx = &ctx;
	RendererScene const& scene = renderContext->GetScene();
	auto& PSPF = *deviceCtx->GetConstantBuffer(ECBFrequency::PS_PerFrame);

	if (layer == EShadingLayer::BASE)
	{
		PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(scene.GetAmbientLight());
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
		auto const& light = scene.GetDirLights()[index];
		PFPSDirLight perFrameData;
		Zero(perFrameData);
		perFrameData.directionalCol = vec3(light.colour);
		perFrameData.directionalDir = light.dir;
		perFrameData.world2Light = lrd.mCamera->GetProjWorld();;
		perFrameData.debugMode = Denum(renderContext->Settings.ShadingDebugMode);
		perFrameData.debugGrayscaleExp = renderContext->Settings.DebugGrayscaleExp;
		perFrameData.brdf = renderContext->mBrdf;
		PSPF.WriteData(perFrameData);
		break;
	}
	case EShadingLayer::SPOTLIGHT:
	{
		auto const& light = scene.GetSpotLights()[index];
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
		auto const& light = scene.GetPointLights()[index];
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
	renderContext->TextureManager.SetTexture(E_TS_SHADOWMAP, lrd.mShadowMap);

}

}
