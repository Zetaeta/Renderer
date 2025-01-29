#include "SsrPass.h"
#include "render/Shader.h"
#include "render/RenderContext.h"
#include "render/shaders/ShaderDeclarations.h"
#include "imgui.h"

namespace rnd
{
struct SSRParameters
{
	mat4 projWorld;
	mat4 screen2World;
	float reflectionThreshold;
	float maxDist;
	float traceResolution;
	uint maxRefineSteps;
	float hitBias;
	uint2 debugPixel;
};

class SSR_CS : public ComputeShader
{
public:
	using Permutation = PermutationDomain<PixelDebuggingSwitch>;
	DECLARE_SHADER(SSR_CS);
};

DEFINE_SHADER(SSR_CS, "ScreenSpaceReflections", "SSR_CS");

class SSR_PS : public PixelShader
{
public:
	using Permutation = PermutationDomain<PixelDebuggingSwitch>;
	DECLARE_SHADER(SSR_PS);
};

DEFINE_SHADER(SSR_PS, "ScreenSpaceReflections", "SSR_PS");

SsrPass::SsrPass(RenderContext* rCtx, RGShaderResources&& srvs, RGRenderTargetRef renderTarget)
:RenderPass(rCtx, "ScreenSpaceReflections"), mSrvs(std::move(srvs)), mRT(renderTarget)
{
}

void SsrPass::Execute(IRenderDeviceCtx& deviceCtx)
{
	auto& renderCtx = *mRCtx;
	SSRParameters cb;
	cb.projWorld = renderCtx.GetCamera().GetProjWorld();
	cb.screen2World = renderCtx.GetCamera().GetInverseProjWorld();
	cb.reflectionThreshold  = reflectionThreshold;
	cb.maxDist = maxDist;
	cb.traceResolution = traceResolution;
	cb.maxRefineSteps = maxRefineSteps;
	cb.hitBias = hitBias;
	cb.debugPixel = renderCtx.Settings.DebugPixel;

	ScopedCBClaim cbv(renderCtx.GetCBPool(), cb);
	IRenderDeviceCtx* context = &deviceCtx;
//	uint2 screenSize = renderCtx.GetPrimaryRenderSize();

//	constexpr uint3 numThreads(8,8,1);
	
	context->SetBlendMode(EBlendState::COL_OVERWRITE | EBlendState::ALPHA_UNTOUCHED);
	context->SetDepthMode(EDepthMode::Disabled);
	context->SetConstantBuffers(EShaderType::Pixel, Single<CBHandle const>(cbv));
	context->SetShaderResources(EShaderType::Pixel, mSrvs.ResolvedViews);
//	context->SetUAVs(EShaderType::Compute, Single(mSceneColourUav));
	context->SetVertexLayout(-1);
	context->SetVertexShader(renderCtx.GetShader<PostProcessVS>());
	auto tri = Device()->BasicMeshes.GetFullScreenTri();
	context->SetVertexLayout(tri->GetVertexAttributes());
	SSR_PS::Permutation perm;
	perm.Set<PixelDebuggingSwitch>(mDebugPixel && renderCtx.Settings.EnablePixelDebug);
	context->SetPixelShader(renderCtx.GetShader<SSR_PS>(perm));

	context->SetRTAndDS(mRT.ResolvedRT, nullptr);
	context->SetUAVs(EShaderType::Pixel, Single<UnorderedAccessView const>(renderCtx.GetPixelDebugUav()), 1);
	context->DrawMesh(tri);
	context->ClearResourceBindings();

}

void SsrPass::Build(RGBuilder& builder)
{
	mSrvs.Resolve(builder);
	mRT.Resolve(builder);
}

void SsrPass::AddControls()
{
	ImGui::DragFloat("Resolution", &traceResolution, 0.1f, 0, 1.f);
	ImGui::DragFloat("Reflection threshold", &reflectionThreshold, 0.1f, 0, 1.f);
	ImGui::DragFloat("Max reflection dist", &maxDist, 1.f, 0, 100.f);
	ImGui::DragFloat("Hit bias", &hitBias, 0.001f, -1.f, 1.f, "%.7f");
}

}
