#include "DeferredShadingPass.h"
#include "render/shaders/ShaderDeclarations.h"
#include "render/ShadingCommon.h"

namespace rnd
{

struct DeferredShadingPS : public PixelShader
{
	__declspec(align(16))
	struct CBPerInst
	{
		mat4 screen2World;
		vec3 cameraPos;
		float padding;
		uint2 screenSize;
	};

	struct PointLight : SHADER_PERM_BOOL("POINT_LIGHT");
	struct DirLight : SHADER_PERM_BOOL("DIR_LIGHT");
	struct SpotLight : SHADER_PERM_BOOL("SPOTLIGHT");
	struct BaseLayer : SHADER_PERM_BOOL("BASE_LAYER");
	struct AmbientOcclusion : SHADER_PERM_BOOL("AMBIENT_OCCLUSION");

	using Permutation = PermutationDomain<PointLight, DirLight, SpotLight, BaseLayer, AmbientOcclusion>;
	DECLARE_SHADER(DeferredShadingPS)
};

DEFINE_SHADER(DeferredShadingPS, "DeferredShading_PS", "main");

DeferredShadingPass::DeferredShadingPass(RenderContext* rCtx, Camera::Ref camera, IRenderTarget::Ref dest, SRVType sceneColour, SRVType sceneNormal, SRVType sceneEmissive, SRVType sceneDepth, SRVType ambientOcclusion)
:RenderPass(rCtx, "DeferredShading"), mSRVs({sceneColour, sceneNormal, sceneEmissive, sceneDepth, ambientOcclusion}),
	mRenderTarget(dest), mCamera(camera)
{
	{
		PostProcessVS::Permutation perm;
		perm.Set<PostProcessVS::UseUVs>(true);
		mVertexShader = rCtx->GetShader<PostProcessVS>(perm);
	}

	for (u32 i = 0; i < Denum(EShadingLayer::SHADING_COUNT); ++i)
	{
		DeferredShadingPS::Permutation perm;
		perm.Set<DeferredShadingPS::AmbientOcclusion>(true);
		switch (EShadingLayer(i))
		{
		case EShadingLayer::BASE:
			perm.Set<DeferredShadingPS::BaseLayer>(true);
			break;
		case EShadingLayer::DIRLIGHT:
			perm.Set<DeferredShadingPS::DirLight>(true);
			break;
		case EShadingLayer::POINTLIGHT:
			perm.Set<DeferredShadingPS::PointLight>(true);
			break;
		case EShadingLayer::SPOTLIGHT:
			perm.Set<DeferredShadingPS::SpotLight>(true);
			break;
		default:
			break;
		}
		mPixelShader[i] = rCtx->GetShader<DeferredShadingPS>(perm);
	}
}

void DeferredShadingPass::Execute(RenderContext& renderCtx)
{

	auto context = DeviceCtx();
	context->SetRTAndDS(mRenderTarget, nullptr);
//	context->ClearRenderTarget(mRenderTarget, )
	context->SetBlendMode(EBlendState::COL_OVERWRITE);
	context->SetViewport(mRenderTarget->Desc.Width, mRenderTarget->Desc.Height);
	context->SetVertexShader(mVertexShader);
	Vector<ResourceView> srvs = mSRVs.ResolvedViews;
	srvs.emplace_back(nullptr);
	auto tri = Device()->BasicMeshes.GetFullScreenTri();

	DeferredShadingPS::CBPerInst cbuf;
	cbuf.screen2World = mCamera->GetInverseProjWorld();
	cbuf.screenSize = mRCtx->GetPrimaryRenderSize();
	cbuf.cameraPos = mCamera->GetPosition();
	renderCtx.DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerInstance, sizeof(DeferredShadingPS::CBPerInst))->WriteData(cbuf);
	context->SetDepthMode(EDepthMode::Disabled);
	context->SetBlendMode(EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE);
	ZE_REQUIRE(tri);
	context->SetVertexLayout(tri->GetVertexAttributes());

	{
		SetupShadingLayer(mRCtx, EShadingLayer::BASE, 0);
		context->SetShaderResources(EShaderType::Pixel, srvs);
		context->SetPixelShader(mPixelShader[0]);
		context->DrawMesh(tri);
	}
	context->SetBlendMode(EBlendState::COL_ADD | EBlendState::ALPHA_MAX);

	for (u8 l = 0; l < Denum(ELightType::COUNT); ++l)
	{
		context->SetPixelShader(mPixelShader[l + 1]);
		ELightType lightType = EnumCast<ELightType>(l);
		EShadingLayer layer = GetLightLayer(lightType);
		if (//Settings.LayersEnabled[Denum(layer)] &&
			mRCtx->Settings.LayersEnabled[Denum(layer)])
		{
			for (int i = 0; i < mRCtx->GetLightData(lightType).size(); ++i)
			{
				SetupShadingLayer(mRCtx, layer, i);
				LightRenderData const& lrd = mRCtx->GetLightData(GetLightFromLayer(layer), i);
				srvs.back().Resource = lrd.mShadowMap;
				context->SetShaderResources(EShaderType::Pixel, srvs);

				context->DrawMesh(tri);
			}
		}
	}
	context->ClearResourceBindings();
}

void DeferredShadingPass::Build(RGBuilder& builder)
{
	mSRVs.Resolve(builder);
}

}
