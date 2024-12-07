#include "SceneRenderPass.h"
#include "dx11/DX11Ctx.h"
#include "dx11/DX11Renderer.h"
#include <core/Matrix.h>

namespace rnd
{

 SceneRenderPass::SceneRenderPass(RenderContext* ctx, Name passName, Camera::Ref camera, IRenderTarget::Ref renderTarget, IDepthStencil::Ref depthStencil,
						EMatType accepts /*= E_MT_ALL*/, EShadingLayer layer /*= EShadingLayer::NONE*/,
						ERenderPassMode mode /*= ERenderPassMode::IMMEDIATE*/)
		: RenderPass(ctx, std::move(passName)), mAcceptedMatTypes(accepts), mLayer(layer), mRenderTarget(renderTarget), mDepthStencil(depthStencil)
		, mViewCam(camera), mMode(mode)
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
	ZE_ASSERT(mViewCam->GetCameraType() == ECameraType::CUBE);
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
#if ZE_BUILD_EDITOR
	if (data.component->Selected)
	{
		DeviceCtx()->SetStencilState({EStencilMode::Overwrite | EStencilMode::IgnoreDepth | EStencilMode::UseBackFace, 250});
	}
#endif
	mRCtx->DrawPrimitive(data.mesh, data.component->GetWorldTransform(), projWorld,
						mMatOverride ? mMatOverride : nullptr, mLayer);
#if ZE_BUILD_EDITOR
	if (data.component->Selected)
	{
		DeviceCtx()->SetStencilState({EStencilMode::Disabled, 0});
	}
#endif
	return;
}

void SceneRenderPass::Execute(RenderContext& renderCtx)
{
	RenderScene(renderCtx.GetScene());
}

}
