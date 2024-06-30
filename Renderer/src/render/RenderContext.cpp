#include "RenderContext.h"
#include "SceneRenderPass.h"
#include "scene/Scene.h"
#include "dx11/DX11Renderer.h"
#include "ForwardRenderPass.h"
#include "ShadowmapsPass.h"
#include "RenderCubemap.h"
#include <imgui.h>

namespace rnd
{

RenderContext::RenderContext(DX11Ctx* ctx, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds)
		: mCtx(ctx)
{
	mCtx = ctx;
	mCtx->mRCtx = this;
	mDevice = mCtx->m_Renderer;
	mDeviceCtx = mCtx->m_Renderer;
	mPasses.emplace_back(std::make_unique<ShadowmapsPass>(mCtx));
	mPasses.emplace_back(std::make_unique<ForwardRenderPass>(this, "ForwardRender", camera, rt, ds));
	mPasses.emplace_back(std::make_unique<RenderCubemap>(EFlatRenderMode::BACK, "Background", mCtx->m_Renderer->m_BG.get()));
	mDebugCubePass = static_cast<RenderCubemap*>(mPasses.emplace_back(std::make_unique<RenderCubemap>(EFlatRenderMode::FRONT, "DebugCube")).get());
	memset(mLayersEnabled, 1, Denum(EShadingLayer::COUNT));
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

Camera::ConstRef LightRenderData::GetCamera() const
{
	return mCamera.get();
}

Camera::Ref LightRenderData::GetCamera()
{
	return mCamera.get();
}

}
