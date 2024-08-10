#include "RenderContext.h"
#include "SceneRenderPass.h"
#include "scene/Scene.h"
#include "dx11/DX11Renderer.h"
#include "ForwardRenderPass.h"
#include "ShadowmapsPass.h"
#include "RenderCubemap.h"
#include <imgui.h>
#include <editor/HighlightSelectedPass.h>

namespace rnd
{

RenderContext::RenderContext(IRenderDeviceCtx* DeviceCtx, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds)
		: mCamera(camera), mMainDS(ds), mMainRT(rt)
	{
	mDevice = DeviceCtx->Device;
	mDeviceCtx = DeviceCtx;
	mPasses.emplace_back(std::make_unique<ShadowmapsPass>(this));
	mPasses.emplace_back(std::make_unique<ForwardRenderPass>(this, "ForwardRender", camera, rt, ds));
	mPasses.emplace_back(std::make_unique<RenderCubemap>(this, EFlatRenderMode::BACK, "Background", static_cast<dx11::DX11Renderer*>(DeviceCtx)->m_BG.get()));
#if RBUILD_EDITOR
	mPasses.emplace_back(MakeOwning<HighlightSelectedPass>(this));
#endif
	mDebugCubePass = static_cast<RenderCubemap*>(mPasses.emplace_back(std::make_unique<RenderCubemap>(this, EFlatRenderMode::FRONT, "DebugCube")).get());
	memset(mLayersEnabled, 1, Denum(EShadingLayer::COUNT));
}

 RenderContext::~RenderContext()
{
	 mPasses.clear();
	for (auto& lrds : mLightData)
	{
		 lrds.clear();
	}
 }

void RenderContext::RenderFrame(Scene const& scene)
{
	mScene = &scene;
	SetupLightData();
	for (auto& pass : mPasses)
	{
		if (pass->IsEnabled())
		{
			pass->RenderFrame(*this);
		}
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
			desc.flags = TF_DEPTH;
			desc.height = desc.width = 1024;
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
			desc.flags = TF_DEPTH;
			desc.height = desc.width = 1024;
			desc.DebugName = debugName;
			lrd.mShadowMap = mDevice->CreateTexture2D(desc);
		}
		lrd.mCamera = std::make_unique<Camera>(cameraType, lightPos);
		if (cameraType == ECameraType::ORTHOGRAPHIC)
		{
			lrd.mCamera->SetViewExtent(mDirShadowmapScale, mDirShadowmapScale);
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
				lrd.mCamera->SetViewExtent(mDirShadowmapScale, mDirShadowmapScale);
			}
		}
	}
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
	else if (auto& deviceMat = mat.DeviceMat)
	{
		matArch = deviceMat->Archetype;
	}
	else
	{
		RASSERT(false);
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
				CBLayout::Ref layout = perInstInfo.Layout;
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
		desc.format = ETextureFormat::D24_Unorm_S8_Uint;
		desc.width = size.x;
		desc.height = size.y;
		desc.flags = TF_DEPTH;
		tempDS = mDevice->CreateTexture2D(desc)->GetDS();
	}
	return tempDS;
}

Camera::ConstRef LightRenderData::GetCamera() const
{
	return mCamera.get();
}

Camera::Ref LightRenderData::GetCamera()
{
	return mCamera.get();
}

}
