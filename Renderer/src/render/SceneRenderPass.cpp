#include "SceneRenderPass.h"
#include "dx11/DX11Ctx.h"
#include "dx11/DX11Renderer.h"
#include <core/Matrix.h>

namespace rnd
{

 SceneRenderPass::SceneRenderPass(RenderContext* ctx, Name passName, Camera::Ref camera, IRenderTarget::Ref renderTarget, IDepthStencil::Ref depthStencil,
						EMatType accepts /*= E_MT_ALL*/, EShadingLayer layer /*= EShadingLayer::NONE*/,
						ERenderPassMode mode /*= ERenderPassMode::IMMEDIATE*/)
		: RenderPass(ctx), mAcceptedMatTypes(accepts), mLayer(layer), mRenderTarget(renderTarget), mDepthStencil(depthStencil)
		, mViewCam(camera), mMode(mode), mPassName(passName)
	{
}

bool SceneRenderPass::Accepts(SceneComponent const* sceneComp, MeshPart const* mesh) const
{
	auto const& mat = sceneComp->GetOwner()->GetScene().GetMaterial(mesh->material);
	if ((mat.GetMatType() & mAcceptedMatTypes) == 0)
	{
		return false;
	}
	return true;
}

void SceneRenderPass::RenderScene(Scene const& scene)
{
	mScene = &scene;
	if (mMode == ERenderPassMode::IMMEDIATE)
	{
		BeginRender();
	}
	mScene->ForEach<StaticMeshComponent>([&](StaticMeshComponent const& smc) {
		auto cmesh = smc.GetMesh();
		auto const& trans = smc.GetWorldTransform();
		if (!IsValid(cmesh))
		{
			return;
		}

		for (auto const& mesh : cmesh->components)
		{
			Accept(&smc, &mesh);
		}
	});
	OnCollectFinished();
}

void SceneRenderPass::Accept(SceneComponent const* sceneComp, MeshPart const* mesh)
{
	if (IsDeferred())
	{
		mBuffer.emplace_back(DrawData{ mesh, sceneComp });
	}
	else
	{
		Draw(DrawData{ mesh, sceneComp });
	}
}

void SceneRenderPass::BeginRender()
{
	int rtIdx = -1, dsIdx = -1;
	if (mViewCam->GetCameraType() != ECameraType::CUBE)
	{
		DeviceCtx()->SetRTAndDS(mRenderTarget, mDepthStencil);
	}
	if (mRenderTarget)
	{
		auto const& rtDesc = mRenderTarget->GetDesc();
		DeviceCtx()->SetViewport(rtDesc.Width, rtDesc.Height);
	}
	else if (mDepthStencil)
	{
		auto const& dsDesc = mDepthStencil->GetDesc();
		DeviceCtx()->SetViewport(dsDesc.Width, dsDesc.Height);
	}

	if (mMatOverride)
	{
		mMatOverride->Bind(*mRCtx, mLayer);
	}

	auto& vsCB = static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerFrameVSCB();
	vsCB.GetCBData()["cameraPos"] |= mViewCam->GetPosition();
	if (auto w2l = vsCB.GetCBData()["world2Light"])
	{
		w2l = mViewCam->WorldToCamera();
	}
	if (auto w2l = vsCB.GetCBData()["screen2World"])
	{
		w2l = mViewCam->GetInverseProjWorld();
	}

	vsCB.Update();
	SetupShading();
}

void SceneRenderPass::SetupShading()
{
}

void SceneRenderPass::SetCubeRTAndDS(u8 face)
{
	RASSERT(mViewCam->GetCameraType() == ECameraType::CUBE);
	u32 rtFace = 0;
	u32 dsFace = 0;
	if (mRenderTarget != nullptr && (mRenderTarget->GetDesc().Dimension == ETextureDimension::TEX_CUBE))
	{
		rtFace = face;
		auto const& rtDesc = mRenderTarget->GetDesc();
		DeviceCtx()->SetViewport(float(rtDesc.Width), float(rtDesc.Height));
	}
	if (mDepthStencil != nullptr && (mDepthStencil->GetDesc().Dimension == ETextureDimension::TEX_CUBE))
	{
		dsFace = face;
		auto const& dsDesc = mDepthStencil->GetDesc();
		DeviceCtx()->SetViewport(dsDesc.Width, dsDesc.Height);
	}

	DeviceCtx()->SetRTAndDS(mRenderTarget, mDepthStencil, rtFace, dsFace);
} 

void SceneRenderPass::RenderBuffer()
{
	BeginRender();
	if (SPLIT_DIRECTIONS && mViewCam->GetCameraType() == ECameraType::CUBE)
	{
		for (u8 face = 0; face<CUBE_FACES; ++face)
		{
			SetCubeRTAndDS(face);
			mat4 const& projection = mViewCam->GetCubeProjections()[face];
			for (auto const& drawData : mBuffer)
			{
				if (mMatOverride)
				{
					mMatOverride->Bind(*mRCtx, mLayer);
				}
				DrawSingle(drawData, projection, projection * mViewCam->WorldToCamera());
			}
		}
	}
	else
	{
		ProcessBuffer();
	}
}

void SceneRenderPass::ProcessBuffer()
{
	for (auto const& dd : mBuffer)
	{
		Draw(dd);
	}
}

void SceneRenderPass::Draw(DrawData const& data)
{
	mat4 view = mViewCam->GetView();
	if (!SPLIT_DIRECTIONS && mViewCam->GetCameraType() == ECameraType::CUBE)
	{
		for (int face=0; face<6; ++face)
		{
			mat4 projection = mViewCam->GetCubeProjections()[face];
			DrawSingle(data, projection, projection * mViewCam->WorldToCamera());
		}
	}
	else
	{
		DrawSingle(data, mViewCam->GetProjection(), mViewCam->GetProjection() * mViewCam->WorldToCamera());
	}
}

void SceneRenderPass::DrawSingle(DrawData const& data, mat4 const& projection, mat4 const& projWorld)
{
	mRCtx->SetScreenObjId(data.component->GetScreenId());
	mRCtx->DrawPrimitive(data.mesh, data.component->GetWorldTransform(), projWorld,
						mMatOverride ? mMatOverride : nullptr, mLayer);
	return;
	MaterialArchetype* matArch = nullptr;

	Material const& mat = mRCtx->GetScene().GetMaterial(data.mesh->material);
	if (mMatOverride)
	{
		matArch = mMatOverride->Archetype;
	}
	else 
	{
		auto& deviceMat = mat.DeviceMat;
		if (!deviceMat || mat.NeedsUpdate())
		{
			DeviceCtx()->PrepareMaterial(data.mesh->material);
		}
		if (deviceMat)
		{
			matArch = deviceMat->Archetype;
		}
	}

	if (matArch)
	{
		if (DataLayout::Ref layout = matArch->CBData[Denum(ECBFrequency::PS_PerInstance)].Layout)
		{
			IConstantBuffer* psPerInst = DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerInstance, layout->GetSize());
			ConstantBufferData& cbData = psPerInst->Data();
			cbData.SetLayout(layout);
			cbData[CB::colour] |= mat.colour;
			cbData[CB::emissiveColour] |= mat.emissiveColour;
			cbData[CB::roughness] |= mat.roughness;
			cbData[CB::metalness] |= mat.metalness;
			cbData[CB::ambdiffspec] |= vec3 {1, mat.diffuseness, mat.specularity};
			cbData[CB::useNormalMap] |= mat.normal->IsValid();
			cbData[CB::useEmissiveMap] |= mat.emissiveMap->IsValid();
			cbData[CB::useRoughnessMap] |= mat.roughnessMap->IsValid();
			cbData[CB::alphaMask] |= mat.mask;
			cbData[CB::screenObjectId] |= data.component->GetScreenId();
			psPerInst->Update();
		}
	}

	Transform const& trans = data.component->GetWorldTransform();
	dx11::PerInstanceVSData PIVS;
	PIVS.model2ShadeSpace = mat4(trans);
	PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
	PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

	static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerInstanceVSCB().WriteData(PIVS);
	DeviceCtx()->DrawMesh(*data.mesh, mLayer, mMatOverride == nullptr);
}

void SceneRenderPass::Execute(RenderContext& renderCtx)
{
	RenderScene(renderCtx.GetScene());
}

}
