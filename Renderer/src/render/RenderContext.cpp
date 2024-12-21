#include "RenderContext.h"
#include "SceneRenderPass.h"
#include "scene/Scene.h"
#include "dx11/DX11Renderer.h"
#include "ForwardRenderPass.h"
#include "ShadowmapsPass.h"
#include "RenderCubemap.h"
#include <imgui.h>
#include <editor/HighlightSelectedPass.h>
#include "Shader.h"
#include "passes/PostProcessPass.h"
#include "shaders/ShaderDeclarations.h"
#include "WavyEffectPPPass.h"
#include "passes/GBufferPass.h"
#include "passes/DeferredShadingPass.h"
#include "passes/SSAOPass.h"
#include "passes/SsrPass.h"

namespace rnd
{

RenderContext::RenderContext(IRenderDeviceCtx* DeviceCtx, Camera::Ref camera, IDeviceTexture::Ref target, RenderSettings const& settings)
	: mCamera(camera), mRGBuilder(DeviceCtx->Device), Settings(settings)
{
	mTarget = target;
	mDeviceCtx = DeviceCtx;
	mDevice = mDeviceCtx->Device;
	SetupRenderTarget();
	SetupPasses();
}

 RenderContext::~RenderContext()
{
	mPasses.clear();
	for (auto& lrds : mLightData)
	{
		lrds.clear();
	}
 }

void RenderContext::SetupRenderTarget()
 {
	DeviceTextureDesc dsDesc = mTarget->Desc;

	DeviceTextureDesc primaryRTDesc = mTarget->Desc;
	primaryRTDesc.Format = ETextureFormat::RGBA8_Unorm_SRGB;
	primaryRTDesc.DebugName = "Primary RT";
	primaryRTDesc.Flags = ETextureFlags::TF_RenderTarget | ETextureFlags::TF_SRGB | ETextureFlags::TF_SRV;
	mPrimaryTarget = mDevice->CreateTexture2D(primaryRTDesc);

	if (mUseMSAA)
	{
		DeviceTextureDesc msaaRTDesc = mTarget->Desc;
		msaaRTDesc.DebugName = "MSAA RT";
		msaaRTDesc.SampleCount = msaaSampleCount;
		msaaRTDesc.Flags = ETextureFlags::TF_RenderTarget | ETextureFlags::TF_SRGB;
		mMsaaTarget = mDevice->CreateTexture2D(msaaRTDesc);
		mMainRT = mMsaaTarget->GetRT();

		dsDesc.SampleCount = msaaSampleCount;
	}
	else
	{
		mMainRT = mPrimaryTarget->GetRT();
		mMsaaTarget = nullptr;
		//mMainRT = mTarget->GetRT();
	}

	// Second target for postprocess effects
	{
		DeviceTextureDesc ppDesc = mTarget->Desc;
		ppDesc.Flags |= TF_SRGB;
		ppDesc.DebugName = "PostProcess RT";
		mPPTarget = mDevice->CreateTexture2D(ppDesc);
	}

	{
	
		dsDesc.Flags = ETextureFlags::TF_DEPTH | ETextureFlags::TF_StencilSRV;
		dsDesc.DebugName = "MainDepthStencil";
		dsDesc.Format = ETextureFormat::D24_Unorm_S8_Uint;
		mDSTex = mDeviceCtx->Device->CreateTexture2D(dsDesc);
		mMainDS = mDSTex->GetDS();
	}
 }

 void RenderContext::SetupPasses()
 {
	tempRemember.clear();
	mPasses.clear();
	mPPPasses.clear();
	mPasses.emplace_back(std::make_unique<ShadowmapsPass>(this));
	mPasses.emplace_back(std::make_unique<ForwardRenderPass>(this, "ForwardRender", mCamera, mMainRT, mMainDS));
	mPasses.emplace_back(std::make_unique<RenderCubemap>(this, EFlatRenderMode::BACK, "Background", static_cast<dx11::DX11Renderer*>(mDeviceCtx)->m_BG.get()));

	DeviceTextureDesc gbAlbedoDesc = mTarget->Desc;
	gbAlbedoDesc.DebugName = "GBuffer";
	gbAlbedoDesc.Flags = TF_RenderTarget | TF_SRV;
	gbAlbedoDesc.Format = ETextureFormat::RGBA8_Unorm_SRGB;
	auto gbAlbedo = mRGBuilder.MakeTexture2D(gbAlbedoDesc);

	DeviceTextureDesc gbNormalDesc = mTarget->Desc;
	gbNormalDesc.DebugName = "GBuffer Normal";
	gbNormalDesc.Flags = TF_RenderTarget | TF_SRV;
	gbNormalDesc.Format = ETextureFormat::RGBA8_Unorm;
	auto gbNormal = mRGBuilder.MakeTexture2D(gbNormalDesc);

	DeviceTextureDesc gbDSDesc = mTarget->Desc;
	gbDSDesc.DebugName = "GBuffer Depth";
	gbDSDesc.Flags = TF_DEPTH | TF_SRV;
	gbDSDesc.Format = ETextureFormat::D24_Unorm_S8_Uint;
	auto gbDS = mRGBuilder.MakeTexture2D(gbDSDesc);

	DeviceTextureDesc aoDesc = mTarget->Desc;
	aoDesc.Format = ETextureFormat::R16_Float;
	aoDesc.Flags = TF_SRV | TF_UAV;
	aoDesc.DebugName = "SSAO";
	auto aoTex = mRGBuilder.MakeTexture2D(aoDesc);

	DeviceTextureDesc pixDebugDesc = mTarget->Desc;
	aoDesc.Format = ETextureFormat::RGBA8_Unorm;
	aoDesc.Flags = TF_SRV | TF_UAV;
	aoDesc.DebugName = "Pixel debug";
	mPixelDebugTex = mRGBuilder.MakeTexture2D(aoDesc);

	mPasses.emplace_back(std::make_unique<GBufferPass>(this, "GBuffer", mCamera, gbAlbedo, gbNormal, gbDS));

	if (Settings.AmbientOcclusion)
	{
		AddPass<SSAOPass>(RGShaderResources({gbDS, gbNormal}), RGUnorderedAccessViews({aoTex}));
	}

	mPasses.emplace_back(std::make_unique<DeferredShadingPass>(this, mCamera, mMainRT, gbAlbedo, gbNormal, nullptr, gbDS, aoTex));
#if ZE_BUILD_EDITOR
//	mPasses.emplace_back(MakeOwning<HighlightSelectedPass>(this));
#endif
	mDebugCubePass = static_cast<RenderCubemap*>(mPasses.emplace_back(std::make_unique<RenderCubemap>(this, EFlatRenderMode::FRONT, "DebugCube")).get());
	memset(Settings.LayersEnabled, 1, Denum(EShadingLayer::Count));


	mPingPongHandle = mRGBuilder.MakeFlipFlop(mPrimaryTarget, mPPTarget);
	auto stencilHdl = mRGBuilder.AddFixedSRV({mDSTex, SRV_StencilBuffer});
	mPPPasses.push_back(MakeOwning<SsrPass>(this, RGShaderResources({gbAlbedo, gbNormal, gbDS, mPingPongHandle}), mPingPongHandle));
	mPPPasses.push_back(MakeOwning<PostProcessPass>(this, GetShader<OutlinePPPS>(), mPingPongHandle, RGShaderResources({mPingPongHandle, stencilHdl}), "Outline"));
	mPPPasses.emplace_back(MakeOwning<WavyEffectPPPass>(this, mPingPongHandle, RGShaderResources({mPingPongHandle})))->SetEnabled(false);

	BuildGraph();
 }

void RenderContext::SetupPostProcess()
{
}


 void RenderContext::RenderFrame(Scene const& scene)
{
	mScene = &scene;
	SetupLightData();
	mDeviceCtx->ClearRenderTarget(mMainRT, { 0.45f, 0.55f, 0.60f, 1.00f });
	mDeviceCtx->ClearDepthStencil(mMainDS, EDSClearMode::DEPTH_STENCIL, 1.f);
	mDeviceCtx->ClearUAV(mPixDebugUav, vec4{0.f});
	for (auto& pass : mPasses)
	{
		if (pass->IsEnabled())
		{
			pass->ExecuteWithProfiling(*this);
		}
	}


	if (mUseMSAA)
	{
		mDeviceCtx->ResolveMultisampled({ mPrimaryTarget }, {mMsaaTarget});
	}

	Postprocessing();
	mDeviceCtx->ClearResourceBindings();
}

void RenderContext::BuildGraph()
{
	mRGBuilder.Reset();

	mPixDebugUav = mRGBuilder.GetUAV(mPixelDebugTex);

	for (auto& pass : mPasses)
	{
		if (pass->IsEnabled())
		{
			pass->Build(mRGBuilder);
		}
	}

	for (auto& pass : mPPPasses)
	{
		if (pass->IsEnabled())
		{
			pass->Build(mRGBuilder);
		}
	}
}

rnd::IDeviceTexture::Ref RenderContext::GetPrimaryRT()
{
	return mTarget;
}

void RenderContext::Postprocessing()
{
	for (auto& pass : mPPPasses)
	{
		if (pass->IsEnabled())
		{
			pass->ExecuteWithProfiling(*this);
		}
	}

	if (Settings.EnablePixelDebug)
	{
		ShowPixelDebug();
	}

	if (static_cast<RGFlipFlop*>(mRGBuilder[mPingPongHandle])->IsOdd())
	{
		mDeviceCtx->Copy(mTarget, mPPTarget);
	}
	else
	{
		mDeviceCtx->Copy(mTarget, mPrimaryTarget);
	}
}

LightRenderData RenderContext::CreateLightRenderData(ELightType lightType, u32 lightIdx)
{
	LightRenderData lrd;

	auto light = mScene->GetLight(lightType, lightIdx);
	std::visit([&]<typename TLight>(TLight const* light)
	{
		pos3 lightPos{0};
		ECameraType cameraType;
		if constexpr (TLight::HAS_POSITION)
		{
			lightPos = light->pos;
		}
		if constexpr (std::is_same_v<TLight, PointLight>)
		{
			cameraType = ECameraType::CUBE;
			DeviceTextureDesc desc;
			desc.Flags = TF_DEPTH;
			desc.Height = desc.Width = 1024;
			desc.DebugName = std::format("{} {}", "PointLightShadow", lightIdx);
			lrd.mShadowMap = mDevice->CreateTextureCube(desc);
		}
		else 
		{
			String debugName;
			if (lightType == ELightType::DIR_LIGHT)
			{
				cameraType = ECameraType::ORTHOGRAPHIC;
				debugName = std::format("{} {}", "DirLightShadow", lightIdx);
			}
			else
			{
				cameraType = ECameraType::PERSPECTIVE;
				debugName = std::format("{} {}", "SpotLightShadow", lightIdx);
			}

			DeviceTextureDesc desc;
			desc.Flags = TF_DEPTH | TF_SRV;
			desc.Height = desc.Width = 1024;
			desc.DebugName = debugName;
			desc.Format = ETextureFormat::D32_Float;
			lrd.mShadowMap = mDevice->CreateTexture2D(desc);
		}
		lrd.mCamera = std::make_unique<Camera>(cameraType, lightPos);
		if (cameraType == ECameraType::ORTHOGRAPHIC)
		{
			lrd.mCamera->SetViewExtent(Settings.DirShadowmapScale, Settings.DirShadowmapScale);
		}
	}, light);

	return lrd;
	//switch (lightType)
	//{
	//case ELightType::DIR_LIGHT:
	//{
	//	auto& light = mScene->m_DirLights[lightIdx];
	//	lrd.mCamera = std::make_unique<Camera>(lightType, vec3{0});
	//	DeviceTextureDesc desc;
	//	desc.flags = TF_DEPTH;
	//	desc.height = desc.width = 1024;
	//	lrd.mShadowMap = mDevice->CreateTexture2D(desc);
	//	break;
	//}
	//case ELightType::POINT_LIGHT:
	//case ELightType::SPOTLIGHT:
	//{
	//	auto& light = mScene->m_PointLights[lightIdx];
	//	lrd.mCamera = std::make_unique<Camera>(lightType, light.pos);
	//	break;
	//}
	//{
	//	auto& light = mScene->m_SpotLights[lightIdx];
	//	lrd.mCamera = std::make_unique<Camera>(lightType, light.pos);
	//	break;
	//}
	//default:
	//	break;
	//}
}

void RenderContext::SetupLightData()
{
	mLightData[Denum(ELightType::DIR_LIGHT)].resize(mScene->m_DirLights.size());
	mLightData[Denum(ELightType::SPOTLIGHT)].resize(mScene->m_SpotLights.size());
	mLightData[Denum(ELightType::POINT_LIGHT)].resize(mScene->m_PointLights.size());
	for (ELightType lightType = ELightType::START; lightType < ELightType::COUNT; ++lightType)
	{
		for (u32 i = 0; i < mLightData[Denum(lightType)].size(); ++ i)
		{
			auto& lrd = mLightData[Denum(lightType)][i];
			if (!lrd.IsValid())
			{
				lrd = CreateLightRenderData(lightType, i);
			}

			QuatTransform trans = mScene->GetLightComponent(lightType, i)->GetWorldTransform();
			if (lightType == ELightType::DIR_LIGHT)
			{
				vec3 offset = - mScene->GetLight<DirLight>(i).dir * 100.f;
				trans.SetTranslation(trans.GetTranslation() + offset);
			}
			lrd.GetCamera()->SetTransform(trans);
			if (lrd.GetCamera()->GetCameraType() == ECameraType::ORTHOGRAPHIC)
			{
				lrd.mCamera->SetViewExtent(Settings.DirShadowmapScale, Settings.DirShadowmapScale);
			}
		}
	}
}

uint2 RenderContext::GetPrimaryRenderSize() const
{
	return {mTarget->Desc.Width, mTarget->Desc.Height};
}

int RenderContext::mBrdf = 1;

void RenderContext::SetDebugCube(IDeviceTextureCube* cubemap)
{
	mDebugCube = cubemap;
	mDebugCubePass->SetCubemap(cubemap);
}

void RenderContext::DrawControls()
{
	static char const* const BRDFs[] = { "Blinn-Phong", "GGX" };
	ImGui::Combo("BRDF", &mBrdf, BRDFs, 2, 2);

	bool needsRebuild = false;
	auto passBox = [&needsRebuild, this](RenderPass& pass)
	{
		bool enabled = pass.IsEnabled();
		ImGui::PushID(&pass);
		if (ImGui::Checkbox(pass.GetName().c_str(), &enabled))
		{
			needsRebuild = true;
			pass.SetEnabled(enabled);
		}
		if (enabled)
		{
			ImGui::Text("GPU Time: %f", pass.GetTimer()->GPUTimeMs);
			pass.AddControls();
		}
		ImGui::PopID();
	};

	for (auto& pass : mPasses)
	{
		passBox(*pass);
	}

	for (auto& pass : mPPPasses)
	{
		passBox(*pass);
	}

	bool needsFullRebuild = false;
	if (ImGui::Checkbox("MSAA", &mUseMSAA))
	{
		needsFullRebuild = true;
	}
	ImGui::Checkbox("Ambient occlusion", &Settings.AmbientOcclusion);
	ImGui::Checkbox("Pixel debugging", &Settings.EnablePixelDebug);
	ImGui::DragInt2("Debug pixel", &Settings.DebugPixel.x);

	if (needsFullRebuild)
	{
		SetupRenderTarget();
		SetupPasses();
	}
	else if (needsRebuild)
	{
		BuildGraph();
	}
}

void RenderContext::DrawPrimComp(const PrimitiveComponent* component, const IDeviceMaterial* matOverride /*= nullptr*/, EShadingLayer layer /*= EShadingLayer::BASE*/)
{
	mCurrentId = component->GetScreenId();
	if (const CompoundMesh* mesh = component->GetMesh())
	{
		for (const MeshPart& part : mesh->components)
		{
			DrawPrimitive(&part, component->GetWorldTransform(), mCamera->GetProjWorld(), matOverride);
		}
	}
}

void RenderContext::DrawPrimitive(const MeshPart* primitive, const mat4& transform, const mat4& viewMatrix, const IDeviceMaterial* matOverride /*= nullptr*/, EShadingLayer layer /*= EShadingLayer::BASE*/)
{
	MaterialArchetype* matArch = nullptr;

	Material const& mat = mScene->GetMaterial(primitive->material);
	if (matOverride)
	{
		matArch = matOverride->Archetype;
	}
	else 
	{
		auto& deviceMat = mat.DeviceMat;
		if (!deviceMat || mat.NeedsUpdate())
		{
			DeviceCtx()->PrepareMaterial(primitive->material);
		}
		if (deviceMat)
		{
			matArch = deviceMat->Archetype;
		}
	}

	if (matArch)
	{
		{
			auto const& perInstInfo = matArch->GetCBData(ECBFrequency::PS_PerInstance);
			bool usesPerInstance = perInstInfo.IsUsed;
			bool usesPerFrame = matArch->GetCBData(ECBFrequency::PS_PerFrame).IsUsed;
			IConstantBuffer* cbs[2] = {nullptr, nullptr};
			u32 index = 0;
			if (usesPerFrame)
			{
				cbs[index++] = &static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerFramePSCB();
			}
			if (usesPerInstance)
			{
				DataLayout::Ref layout = perInstInfo.Layout;
				IConstantBuffer* psPerInst = DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerInstance, layout->GetSize());
				psPerInst->SetLayout(layout);
				ConstantBufferData& cbData = psPerInst->Data();
				cbData[CB::colour] |= mat.colour;
				cbData[CB::emissiveColour] |= mat.emissiveColour;
				cbData[CB::roughness] |= mat.roughness;
				cbData[CB::metalness] |= mat.metalness;
				cbData[CB::ambdiffspec] |= vec3 {1, mat.diffuseness, mat.specularity};
				cbData[CB::useNormalMap] |= mat.normal->IsValid();
				cbData[CB::useEmissiveMap] |= mat.emissiveMap->IsValid();
				cbData[CB::useRoughnessMap] |= mat.roughnessMap->IsValid();
				cbData[CB::alphaMask] |= mat.mask;
				cbData[CB::screenObjectId] |= mCurrentId;
				cbData.FillFromSource(mCBOverrides);
				psPerInst->Update();
				cbs[index++] = psPerInst;
			}

			DeviceCtx()->SetConstantBuffers(EShaderType::Pixel, cbs, index);
		}
	}

	dx11::PerInstanceVSData PIVS;
	PIVS.model2ShadeSpace = transform;
	PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
	PIVS.fullTransform = viewMatrix * PIVS.model2ShadeSpace;

	static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerInstanceVSCB().WriteData(PIVS);
	if (matArch)
	{
		auto const& perInstInfo = matArch->GetCBData(ECBFrequency::VS_PerInstance);
		bool usesPerInstance = perInstInfo.IsUsed;
		bool usesPerFrame = matArch->GetCBData(ECBFrequency::VS_PerFrame).IsUsed;
		IConstantBuffer* cbs[2] = {nullptr, nullptr};
		u32 index = 0;
		if (usesPerInstance)
		{
			cbs[index++] = &static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerInstanceVSCB();
		}
		if (usesPerFrame)
		{
			cbs[index++] = &static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerFrameVSCB();
		}
		DeviceCtx()->SetConstantBuffers(EShaderType::Vertex, cbs, index);
	}
	DeviceCtx()->DrawMesh(*primitive, layer, matOverride == nullptr);
}

IDepthStencil::Ref RenderContext::GetTempDepthStencilFor(IRenderTarget::Ref rt)
{
	u32vec2 size {rt->Desc.Width, rt->Desc.Height};
	IDepthStencil::Ref& tempDS = mTempDepthStencils[size];
	if (!tempDS)
	{
		DeviceTextureDesc desc;
		desc.Format = ETextureFormat::D24_Unorm_S8_Uint;
		desc.Width = size.x;
		desc.Height = size.y;
		desc.Flags = TF_DEPTH;
		tempDS = mDevice->CreateTexture2D(desc)->GetDS();
	}
	return tempDS;
}

void RenderContext::ShowPixelDebug()
{
	mDeviceCtx->SetBlendMode(EBlendState::COL_OBSCURE_ALPHA | EBlendState::COL_BLEND_ALPHA | EBlendState::ALPHA_ADD);
	mDeviceCtx->SetDepthMode(EDepthMode::Disabled);

	PostProcessVS::Permutation perm;
	perm.Set<PostProcessVS::UseUVs>(true);
	static auto vs = GetShader<PostProcessVS>(perm);
	static auto ps = GetShader<FlatPS>();

	mDeviceCtx->SetPixelShader(ps);
	mDeviceCtx->SetVertexShader(vs);
	mDeviceCtx->SetShaderResources(EShaderType::Pixel, Single<ResourceView const>(mRGBuilder.GetSRV(mPixelDebugTex)));
	auto tri = mDevice->BasicMeshes.GetFullScreenTri();
	DeviceTextureRef lastRT, _;
	mRGBuilder.GetFlipFlopState(mPingPongHandle, _, lastRT);
	mDeviceCtx->SetRTAndDS(lastRT->GetRT(), IDepthStencil::Ref{});
	mDeviceCtx->DrawMesh(tri);
}

Camera::ConstRef LightRenderData::GetCamera() const
{
	return mCamera.get();
}

Camera::Ref LightRenderData::GetCamera()
{
	return mCamera.get();
}

RenderSettings::RenderSettings()
{
	memset(LayersEnabled, 1, Denum(EShadingLayer::Count));
}

}
