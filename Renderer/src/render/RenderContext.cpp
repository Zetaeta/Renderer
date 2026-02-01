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
#include "scene/StaticMeshComponent.h"
#include "RendererScene.h"
#include "asset/AssetManager.h"
#include "common/ImguiThreading.h"
#include "RenderHelpers.h"
#include "VertexTypes.h"
#include "RenderConstants.h"

namespace rnd
{
RenderSettings  GSettings;

RenderContext::RenderContext(IRenderDeviceCtx* DeviceCtx, RendererScene* scene, Camera::Ref camera, IDeviceTexture::Ref target, RenderSettings const& settings)
	: mCamera(camera), mRGBuilder(DeviceCtx->Device)//, Settings(settings)
	, mScene(scene)
{
	mTarget = target;
	mDeviceCtx = DeviceCtx;
	mDevice = mDeviceCtx->Device;

	TextureRef bgTex = Texture::LoadFrom("content/canyon2.jpg");
	CubemapData cubeData;
	cubeData.Format = ECubemapDataFormat::FoldUp;
	cubeData.Tex = bgTex;
	DeviceTextureDesc desc;
	desc.Format = ETextureFormat::RGBA8_Unorm_SRGB;
	desc.DebugName = "Background";
	desc.ResourceType = EResourceType::TextureCube;
	mBGCube = mDevice->CreateTextureCube(desc, cubeData);

	SetupRenderTarget();
	SetupPasses();

	char const* deviceName = DeviceCtx->Device->GetName();
	mImguiHandle = ThreadImgui::RegisterImguiFunc([this, deviceName]
	{
		ImGui::Begin(std::format("{} RenderContext", deviceName).c_str());
		DrawControls();
		ImGui::End();
	}, mDestructionToken);
}

RenderContext::~RenderContext()
{
	ThreadImgui::UnregisterImguiFunc(mImguiHandle);
	mDestructionToken->Cancel();
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
		msaaRTDesc.Flags = ETextureFlags::TF_RenderTarget | ETextureFlags::TF_SRGB | TF_SRV;
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
		DeviceTextureDesc ppDesc = primaryRTDesc;
		ppDesc.Flags |= TF_SRGB | TF_SRV;
		ppDesc.DebugName = "PostProcess RT";
		mPPTarget = mDevice->CreateTexture2D(ppDesc);
	}

	{
	
		dsDesc.Flags = ETextureFlags::TF_DEPTH | ETextureFlags::TF_StencilSRV | TF_SRV;
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
	auto& builder = mRGBuilder;

	AddPassOpt<ShadowmapsPass>(builder, "Shadowmaps");
	AddPassOpt<ForwardRenderPass>(builder, "ForwardRender", mCamera, mMainRT, mMainDS);
	AddPassOpt<RenderCubemap>(builder, "Background", EFlatRenderMode::BACK, mBGCube.get());
	AddPassOpt<GBufferPass>(builder, "GBuffer", mCamera, gbAlbedo, gbNormal, gbDS);

	if (Settings.AmbientOcclusion)
	{
		AddPassOpt<SSAOPass>(mRGBuilder, "SSAO", RGShaderResources({ gbDS, gbNormal }), RGUnorderedAccessViews({ aoTex }));
	}

	AddPassOpt<DeferredShadingPass>(builder, "Deferred Shading", mCamera, mMainRT, gbAlbedo, gbNormal, nullptr, gbDS, aoTex);
#if ZE_BUILD_EDITOR
//	mPasses.emplace_back(MakeOwning<HighlightSelectedPass>(this));
#endif

	if (mUseMSAA)
	{
		AddPass<LambdaPass>(mRGBuilder, [&](IRenderDeviceCtx& ctx)
		{
			ctx.ResolveMultisampled({ mPrimaryTarget }, {mMsaaTarget});
		});
	}

	mDebugCubePass = AddPassOpt<RenderCubemap>(builder, "DebugCube", EFlatRenderMode::FRONT);
	memset(Settings.LayersEnabled, 1, Denum(EShadingLayer::Count));


	mPingPongHandle = mRGBuilder.MakeFlipFlop(mPrimaryTarget, mPPTarget);
	auto stencilHdl = mRGBuilder.AddFixedSRV({mDSTex, SRV_StencilBuffer});
	AddPassOpt<SsrPass>(builder, "SSR", RGShaderResources({gbAlbedo, gbNormal, gbDS, mPingPongHandle}), mPingPongHandle);
	AddPassOpt<PostProcessPass>(builder, "Outline", GetShader<OutlinePPPS>(), mPingPongHandle, RGShaderResources({ mPingPongHandle, stencilHdl }));
	AddDisabledPass<WavyEffectPPPass>(builder, "Wavy", mPingPongHandle, RGShaderResources({ mPingPongHandle }));

	BuildGraph();
 }

void RenderContext::SetupPostProcess()
{
}


 void RenderContext::RenderFrame(rnd::IRenderDeviceCtx& ctx)
{
	SetupLightData();
//	mDeviceCtx->ClearRenderTarget(mMainRT, { 0.45f, 0.55f, 0.60f, 1.00f });
	ctx.ClearRenderTarget(mMainRT, { 0.f, 0.f, 0.f, 0.00f });
	ctx.ClearDepthStencil(mMainDS, EDSClearMode::DEPTH_STENCIL, 1.f);
	ctx.ClearUAV(mPixDebugUav, vec4{0.f});
	//for (auto& pass : mPasses)
	//{
	//	if (pass->IsEnabled())
	//	{
	//		pass->ExecuteWithProfiling(ctx);
	//	}
	//}

	//if (mUseMSAA)
	//{
	//	ctx.ResolveMultisampled({ mPrimaryTarget }, {mMsaaTarget});
	//}

	mRGBuilder.Execute(*this);
	Postprocessing(ctx);
	ctx.ClearResourceBindings();

	UpdateDebugTextures();
}

 void RenderContext::SetTarget(IDeviceTexture::Ref newTarget)
 {
	 mTarget = newTarget;
 }

void RenderContext::BuildGraph()
{
	mRGBuilder.Compile();
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

void RenderContext::Postprocessing(rnd::IRenderDeviceCtx& ctx)
{
	for (auto& pass : mPPPasses)
	{
		if (pass->IsEnabled())
		{
			pass->ExecuteWithProfiling(ctx);
		}
	}

	if (Settings.EnablePixelDebug)
	{
		ShowPixelDebug(ctx);
	}

	if (auto debugTex = mViewerTex.lock())
	{
		DrawTexture(debugTex, {0,0});
	}

	if (static_cast<RGFlipFlop*>(mRGBuilder[mPingPongHandle])->IsOdd())
	{
		ctx.Copy(mTarget, mPPTarget);
	}
	else
	{
		ctx.Copy(mTarget, mPrimaryTarget);
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
			desc.Flags = TF_DEPTH | TF_SRV;
			desc.Format = ETextureFormat::D32_Float;
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
			desc.Format = ETextureFormat::D32_Float;
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
	mLightData[Denum(ELightType::DIR_LIGHT)].resize(mScene->GetDirLights().size());
	mLightData[Denum(ELightType::SPOTLIGHT)].resize(mScene->GetSpotLights().size());
	mLightData[Denum(ELightType::POINT_LIGHT)].resize(mScene->GetPointLights().size());
	for (ELightType lightType = ELightType::START; lightType < ELightType::COUNT; ++lightType)
	{
		for (u32 i = 0; i < mLightData[Denum(lightType)].size(); ++ i)
		{
			auto& lrd = mLightData[Denum(lightType)][i];
			if (!lrd.IsValid())
			{
				lrd = CreateLightRenderData(lightType, i);
			}

			QuatTransform trans = mScene->GetLightTransform(lightType, i);
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

	auto getName = [](auto& tex)
	{
		return std::format("{}##{}", tex.Desc.DebugName.empty() ? (!!(tex.Desc.Flags & TF_DEPTH) ? "Depth" : "Texture")
										  : tex.Desc.DebugName.c_str(), reinterpret_cast<u64>(&tex));
	};

	auto viewerTex = mViewerTex.lock();
	if (ImGui::BeginCombo("Texture viewer", viewerTex == nullptr ? "None" : getName(*viewerTex).c_str()))
	{
		// None
		{
			bool selected = viewerTex == nullptr;
			if (ImGui::Selectable("None##1", selected))
			{
				viewerTex = nullptr;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		for (auto tex : mDebugTextures)
		{
			auto strongTex = tex.Tex.lock();
			String name = getName(tex);
			bool selected = viewerTex == strongTex;
			if (ImGui::Selectable(name.c_str(), selected) && strongTex)
			{
				mViewerTex = tex.Tex;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	if (ImGui::Button("Full recompile shaders"))
	{
		GetDevice()->ShaderMgr.RecompileAll(true);
	}

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
		if (enabled && pass.GetTimer())
		{
			ImGui::Text("GPU Time: %f", pass.GetTimer()->GPUTimeMs);
			pass.AddControls();
		}
		ImGui::PopID();
	};

	for (auto& pass : mRGBuilder.GetPasses())
	{
		passBox(*pass);
	}

	//for (auto& pass : mPPPasses)
	//{
	//	passBox(*pass);
	//}

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

void RenderContext::DrawPrimitive(const IDeviceIndexedMesh* mesh, const mat4& transform, const mat4& viewMatrix, RenderMaterial* matOverride /*= nullptr*/, EShadingLayer layer /*= EShadingLayer::BASE*/)
{
	MaterialArchetype* matArch = nullptr;

//	Material const& material = AssetManager::Get()->GetMaterial(primitive->material);
	StandardMatProperties const& mat = matOverride->Props();
	TexturedRenderMaterial const* texMat = dynamic_cast<TexturedRenderMaterial const*>(matOverride);
	if (matOverride)
	{
		matArch = matOverride->Archetype;
	}
	else 
	{
		//auto& deviceMat = material.DeviceMat;
		//if (!deviceMat || material.NeedsUpdate())
		//{
		//	DeviceCtx()->PrepareMaterial(primitive->material);
		//}
		//if (deviceMat)
		//{
		//	matArch = deviceMat->Archetype;
		//}
	}
	matOverride->Bind(*this, layer);

	if (matArch)
	{
		auto const& perInstInfo = matArch->GetCBData(ECBFrequency::PS_PerInstance);
		bool usesPerInstance = perInstInfo.IsUsed;
		bool usesPerFrame = matArch->GetCBData(ECBFrequency::PS_PerFrame).IsUsed;
		IConstantBuffer* cbs[2] = {nullptr, nullptr};
		u32 index = 0;
		if (usesPerFrame)
		{
			cbs[index++] = DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerFrame);
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
			cbData[CB::useNormalMap] |= texMat && texMat->m_Normal;
			cbData[CB::useEmissiveMap] |= texMat && texMat->m_Emissive;
			cbData[CB::useRoughnessMap] |= texMat && texMat->m_Roughness;
			cbData[CB::alphaMask] |= mat.mask;
			cbData[CB::screenObjectId] |= mCurrentId;
			cbData.FillFromSource(mCBOverrides);
			psPerInst->Update();
			cbs[index++] = psPerInst;
		}

		DeviceCtx()->SetConstantBuffers(EShaderType::Pixel, cbs);
	}

	dx11::PerInstanceVSData PIVS;
	PIVS.model2ShadeSpace = transform;
	PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
	PIVS.fullTransform = viewMatrix * PIVS.model2ShadeSpace;

	DeviceCtx()->GetConstantBuffer(ECBFrequency::VS_PerInstance)->WriteData(PIVS);
	if (matArch)
	{
		auto const& perInstInfo = matArch->GetCBData(ECBFrequency::VS_PerInstance);
		bool usesPerInstance = perInstInfo.IsUsed;
		bool usesPerFrame = matArch->GetCBData(ECBFrequency::VS_PerFrame).IsUsed;
		IConstantBuffer* cbs[2] = {nullptr, nullptr};
		u32 index = 0;
		if (usesPerInstance)
		{
			cbs[index++] = DeviceCtx()->GetConstantBuffer(ECBFrequency::VS_PerInstance);
		}
		if (usesPerFrame)
		{
			cbs[index++] = DeviceCtx()->GetConstantBuffer(ECBFrequency::VS_PerFrame);
		}
		DeviceCtx()->SetConstantBuffers(EShaderType::Vertex, cbs);
	}

//	DeviceCtx()->DrawMesh(*primitive, layer, matOverride == nullptr);
	DeviceCtx()->DrawMesh(mesh);
}

void RenderContext::DrawTexture(const DeviceTextureRef& tex, const vec2& pos, const vec2& scale /*= vec2(1)*/, EDrawTextureMode mode /*= EDrawTextureMode::CorrectedRatio*/)
{
	auto& deviceCtx = *DeviceCtx();
	deviceCtx.Device->MatMgr.MatTypes()[MAT_2D].Bind(*this, EShadingLayer::BASE, E_MT_OPAQUE);
	
	VS2DCBuff cbuff;
	vec2 texturePos = vec2(pos);// / vec2 {m_Width, m_Height};
	cbuff.pos = texturePos;// vec2(texturePos.x * 2 - 1, 1 - 2 * texturePos.y);
	//if (size.x <= 0)
	//	size.x = mViewerSize.x;
	//if (size.y <= 0)
	//	size.y = mViewerSize.y;
	cbuff.size = vec2(1);// / vec2 {m_Width, m_Height};
	SetConstantBuffer(deviceCtx, EShaderType::Vertex, cbuff);

	PS2DCBuff pbuff;
	if (tex->Desc.Format == ETextureFormat::R32_Uint)
	{
		pbuff.renderMode = 2;
	}
	else if (tex->IsDepthStencil())
	{
		pbuff.renderMode = 1;
	}
	else if (GetNumChannels(tex->Desc.Format) == 1)
	{
		pbuff.renderMode = 3;
	}
	pbuff.exponent = 1;
	SetConstantBuffer(deviceCtx, EShaderType::Pixel, pbuff);
	ResourceView srv{ tex };
	deviceCtx.SetShaderResources(EShaderType::Pixel, Single(srv));

	deviceCtx.SetBlendMode(EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE);
	deviceCtx.SetDepthMode(EDepthMode::Disabled);
	deviceCtx.SetVertexLayout(GetVertAttHdl<FlatVert>());
	deviceCtx.DrawMesh(deviceCtx.Device->BasicMeshes.GetFullscreenSquare());
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

void RenderContext::SetShaderParameters(EShaderType shaderStage, const ShaderParamersInfo& shaderParams,
	ShaderParamStructMeta const& paramStructMeta, void const* paramStruct, char const* debugShaderName)
{
	static Vector<ResourceView> SRVs;
	static Vector<UnorderedAccessView> UAVs;
	auto const numSrvSlots = shaderParams.SRVs.size();
	auto const numUavSlots = shaderParams.UAVs.size();
	ClearToSize(SRVs, numSrvSlots);
	ClearToSize(UAVs, numUavSlots);

	if (numUavSlots > 0)
	{
		for (u32 i = 0; i<numUavSlots; ++i)
		{
			HashString name = shaderParams.UAVs[i].Name;
			if (ShaderParamStructEntryMeta const* param = FindByName(paramStructMeta.UAVs, name))
			{
				DeviceCtx()->SetUAVs(shaderStage, Single(param->Get<UnorderedAccessView>(paramStruct)), i);
			}
			else
			{
				ZE_Ensuref(false, "UAV parameter %s on %s not set", name.c_str(), debugShaderName);
			}
		}
	}

	if (numSrvSlots > 0)
	{
		for (u32 i = 0; i<numSrvSlots; ++i)
		{
			HashString name = shaderParams.SRVs[i].Name;
			if (ShaderParamStructEntryMeta const* param = FindByName(paramStructMeta.SRVs, name))
			{
				DeviceCtx()->SetShaderResources(shaderStage, Single<ResourceView const>(param->Get<ResourceView>(paramStruct)), i);
			}
			else
			{
				ZE_Ensuref(false, "SRV parameter %s on %s not set", name.c_str(), debugShaderName);
			}
		}
	}

	for (u32 cbIdx = 0; cbIdx < shaderParams.ConstantBuffers.size(); ++cbIdx)
	{
		const auto& cbuffer = shaderParams.ConstantBuffers[cbIdx];
		static HashString GlobalsName = "$Globals";

		if (ShaderParamStructEntryMeta const* param = FindByName(paramStructMeta.CBVs, cbuffer.Name))
		{
			if (ZE_ENSURE(param->Size >= cbuffer.Size))
			{
				SetConstantBuffer(*DeviceCtx(), shaderStage, &param->Get<u8 const>(paramStruct), cbuffer.Size, cbIdx);
			}
		}
		else if (cbuffer.Name == GlobalsName || shaderParams.ConstantBuffers.size() == 1)
		{
			// Should we zero it?
			alignas(16) u8 buffer[MAX_CONSTANT_BUFFER_SIZE];
			for (int i = 0; i < cbuffer.VariableNames.size(); ++i)
			{
				if (ShaderParamStructEntryMeta const* param = FindByName(paramStructMeta.Values, cbuffer.VariableNames[i]))
				{
					memcpy(buffer + cbuffer.VariableOffsets[i], static_cast<u8 const*>(paramStruct) + param->Offset, param->Size);
					SetConstantBuffer(*DeviceCtx(), shaderStage, buffer, cbuffer.Size, cbIdx);
				}
			}
		}
	}

	
}

void RenderContext::DispatchInternal(ComputeShader const* shader, ShaderParamStructMeta const& paramStructMeta, void const* paramStruct, ComputeDispatch const& threadGroups)
{
	auto* ctx = DeviceCtx();
	ctx->ClearResourceBindings(EShaderType::Compute);
	ctx->SetComputeShader(shader);
	SetShaderParameters(EShaderType::Compute, shader->GetParamsInfo(), paramStructMeta,
			paramStruct, shader->GetDebugName());
	ctx->DispatchCompute(threadGroups);
}

void RenderContext::ShowPixelDebug(rnd::IRenderDeviceCtx& ctx)
{
	ctx.SetBlendMode(EBlendState::COL_OBSCURE_ALPHA | EBlendState::COL_BLEND_ALPHA | EBlendState::ALPHA_ADD);
	ctx.SetDepthMode(EDepthMode::Disabled);

	PostProcessVS::Permutation perm;
	perm.Set<PostProcessVS::UseUVs>(true);
	static auto vs = GetShader<PostProcessVS>(perm);
	static auto ps = GetShader<FlatPS>();

	ctx.SetPixelShader(ps);
	ctx.SetVertexShader(vs);
	ctx.SetShaderResources(EShaderType::Pixel, Single<ResourceView const>(mRGBuilder.GetSRV(mPixelDebugTex)));
	auto tri = mDevice->BasicMeshes.GetFullScreenTri();
	DeviceTextureRef lastRT, _;
	mRGBuilder.GetFlipFlopState(mPingPongHandle, _, lastRT);
	ctx.SetRTAndDS(lastRT->GetRT(), IDepthStencil::Ref{});
	ctx.DrawMesh(tri);
}

void RenderContext::UpdateDebugTextures()
{
	Vector<TextureDebugInfo> newDebugTextures;
	for (const auto& resource : mDevice->GetResources())
	{
		if (resource->IsTexture())
		{
			auto* tex = static_cast<IDeviceTexture*>(resource);
			newDebugTextures.emplace_back(tex->Desc, std::static_pointer_cast<IDeviceTexture>(tex->shared_from_this()));
		}
	}
	std::scoped_lock lock {mDebugTexMutex};
	std::swap(newDebugTextures, mDebugTextures);
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
