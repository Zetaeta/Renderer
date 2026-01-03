#include "SSAOPass.h"
#include "render/Shader.h"
#include "render/RenderContext.h"
#include "imgui.h"
#include "core/Random.h"


namespace rnd
{
constexpr size_t NOISE_SIZE = 4;
constexpr size_t MAX_SAMPLES = 64;

struct SSAO_CS : public ComputeShader
{
	DECLARE_SHADER(SSAO_CS)

	struct PixelDebugging : SHADER_PERM_BOOL("PIXEL_DEBUGGING");
	using Permutation = PermutationDomain<PixelDebugging>;

	struct FrameData
	{
		mat4 projectionMat;
		mat4 inverseProjection;
		mat4 world2Screen;
		uint2 screenSize;
		float threshold;
		float radius;
		float2 randomRotationVecs[NOISE_SIZE * NOISE_SIZE];
		float4 samples[MAX_SAMPLES];
		uint numSamples;
		int _pad;
		int2 debugPixel;
	};
};

DEFINE_SHADER(SSAO_CS, "SSAO_CS", "main");

//struct DownsampleCS : public ComputeShader
//{
//
//	DECLARE_SHADER(DownsampleCS)
//};
//
//DEFINE_SHADER(DownsampleCS, "Downsample", "main");

struct GaussianBlurCS : public ComputeShader
{
	constexpr static u32 MaxRadius = 17;
	struct BlurData
	{
		uint2 direction;
		uint radius;
		uint _pad;
		float4 coeffs[MaxRadius];
	};
	
	SHADER_PARAMETER_STRUCT_START(Parameters)
		SHADER_PARAMETER_CBV(BlurData, blurData)
		SHADER_PARAMETER_SRV(RWTexture2D<float4>, outTex)
		SHADER_PARAMETER_UAV(Texture2D<float4>, inTex)
	SHADER_PARAMETER_STRUCT_END()

	DECLARE_SHADER(GaussianBlurCS)
};

DEFINE_SHADER(GaussianBlurCS, "GaussianBlur", "main");

static SSAO_CS::FrameData cb;
static GaussianBlurCS:: BlurData blurData;

SSAOPass::SSAOPass(RenderContext* rCtx, RGShaderResources&& srvs, RGUnorderedAccessViews&& uavs)
:RenderPass(rCtx, "SSAO"), mSRVs(std::move(srvs)), mAOTexture(uavs.Handles[0])
{
	DeviceTextureDesc aoDesc = rCtx->GetPrimaryRT()->Desc;
	aoDesc.Format = ETextureFormat::R16_Float;
	aoDesc.Flags = TF_SRV | TF_UAV;
	aoDesc.DebugName = "SSAO 2ndary";
	mSecondaryTexture = rCtx->GraphBuilder().MakeTexture2D(aoDesc);
	mShader = rCtx->GetShader<SSAO_CS>();
	SSAO_CS::Permutation perm;
	perm.Set<SSAO_CS::PixelDebugging>(true);
	mDebugShader = rCtx->GetShader<SSAO_CS>(perm);
	mDownsampler = rCtx->GetShader<GaussianBlurCS>();
	Rerandomize();
	for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i)
	{
		cb.randomRotationVecs[i] = float2(Random::Range(-1, 1), Random::Range(-1, 1));
	}

}

SSAOPass::~SSAOPass()
{
}

void SSAOPass::Execute(IRenderDeviceCtx& deviceCtx)
{
//	Rerandomize();

	IRenderDeviceCtx* context = &deviceCtx;
	auto& renderCtx = *mRCtx;
	context->ClearResourceBindings();
	bool doPixelDebug = mDebugPixel && renderCtx.Settings.EnablePixelDebug;
	UnorderedAccessView uavs[] = {mAOTextureUav, doPixelDebug ? renderCtx.GetPixelDebugUav() : UnorderedAccessView{}};
	context->SetUAVs(EShaderType::Compute, doPixelDebug ? uavs : Single(uavs[0]));
	context->ClearUAV(mAOTextureUav, vec4{1});
	if (doPixelDebug)
	{
		cb.debugPixel = renderCtx.Settings.DebugPixel;
	}
	else
	{
		cb.debugPixel = {-1, -1};
	}
	if (renderCtx.Settings.AmbientOcclusion)
	{
//		Rerandomize();
		context->SetShaderResources(EShaderType::Compute, mSRVs.ResolvedViews);
		if (IDeviceResource* resource = mSRVs.ResolvedViews[0].Resource.get())
		{
			DeviceTextureDesc const& desc = static_cast<IDeviceTexture*>(resource)->Desc;
			cb.screenSize = uint2(desc.Width, desc.Height);
		}
		cb.numSamples = mNumSamples;
		cb.threshold = mThreshold;
		cb.radius = mRadius;
		cb.world2Screen = transpose(inverse(renderCtx.GetCamera().WorldToCamera()));
		cb.projectionMat = renderCtx.GetCamera().GetProjection();
		cb.inverseProjection = renderCtx.GetCamera().GetInverseProjection();

		constexpr uint2 numThreads = {8, 8};
		const ComputeDispatch threadGroups = {DivideRoundUp(cb.screenSize.x, numThreads.x), DivideRoundUp(cb.screenSize.y, numThreads.y), 1};
		{
			ScopedCBClaim ssaoCB(mRCtx->GetCBPool(), cb);
			context->SetConstantBuffers(EShaderType::Compute, Single<CBHandle const>(ssaoCB));
			context->SetComputeShader(doPixelDebug ? mDebugShader : mShader);

			context->DispatchCompute(threadGroups);

		}


		if (blur)
		{
			GaussianBlurCS::Parameters params;
			CalcGaussian();
			//context->SetComputeShader(mDownsampler);
			//context->SetShaderResources(EShaderType::Compute, Single<ResourceView const>({}));
			//context->SetUAVs(EShaderType::Compute, Single(mSecondaryTextureUav));
			//context->SetShaderResources(EShaderType::Compute, Single(mAOTextureIn));
			blurData.direction = {0, 1};
			params.blurData = blurData;
			params.inTex = mAOTextureIn;
			params.outTex = mSecondaryTextureUav;
			mRCtx->Dispatch(mDownsampler, params, threadGroups);
			//{
			//	ScopedCBClaim blurCB1(renderCtx.GetCBPool(), blurData);
			//	context->SetConstantBuffers(EShaderType::Compute, Single<CBHandle const>(blurCB1));
			//	context->DispatchCompute(threadGroups);
			//}
//			mCB = renderCtx.GetCBPool()->AcquireConstantBuffer(ECBLifetime::Dynamic, blurData);

			//context->SetShaderResources(EShaderType::Compute, Single<ResourceView const>({}));
			//context->SetUAVs(EShaderType::Compute, Single(mAOTextureUav));
			//context->SetShaderResources(EShaderType::Compute, Single(mSecondaryTextureIn));
			params.inTex = mSecondaryTextureIn;
			params.outTex = mAOTextureUav;
			params.blurData.direction = {1, 0};
			mRCtx->Dispatch(mDownsampler, params, threadGroups);

		}

	}
	context->UnbindUAVs(EShaderType::Compute, 1);
}

void SSAOPass::Build(RGBuilder& builder)
{
	mSRVs.Resolve(builder);
	mAOTextureIn = builder.GetSRV(mAOTexture);
	mSecondaryTextureIn = builder.GetSRV(mSecondaryTexture);
	mAOTextureUav = builder.GetUAV(mAOTexture);
	mSecondaryTextureUav = builder.GetUAV(mSecondaryTexture);
//	mUAVs.Resolve(builder);
}

void SSAOPass::AddControls()
{
	ImGui::DragFloat("Threshold", &mThreshold, 0.0001f, -1.f, 1.f, "%.7f");
	ImGui::DragFloat("Rad", &mRadius, 0.1f, 0.f, 10.f, "%.2f");
	ImGui::DragInt("Sample count", &mNumSamples, 1.f, 0, MAX_SAMPLES);
	//ImGui::DragInt("Normal type", &normalType, 1.f, 0, 3);
	ImGui::Checkbox("Blur", &blur);
	ImGui::Checkbox("hemisphere", &hemisphere);
	ImGui::Checkbox("pixel debuggin", &mDebugPixel);
	ImGui::DragFloat("Blur stdev", &stdDev, 0.1f, 1.f, 10.f, "%.2f");
	ImGui::DragInt("Blur radius", &gaussRad, 1.f, 2, 17);
	if (ImGui::Button("rand"))
	{
		Rerandomize();
	}
}

void SSAOPass::Rerandomize()
{
	for (int i = 0; i < mNumSamples; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			cb.samples[i][j] = Random::Normal(0, 1.f);
		}

		if (hemisphere)
		{
			cb.samples[i].z	= abs(cb.samples[i].z);
		}
		cb.samples[i].w = 0;
		float length = glm::length(cb.samples[i]);
		if (length > 0)
		{
			cb.samples[i] /= length;
		}
		float scale = float(i) / mNumSamples;
		float scaleMul = lerp(0.1f, 1.f, scale * scale);
		cb.samples[i] *= scaleMul;
	}
}

void SSAOPass::CalcGaussian()
{
	if (gaussRad > GaussianBlurCS::MaxRadius )
	{
		gaussRad = GaussianBlurCS::MaxRadius;
	}

	const double constantFactor = 1. / (sqrt(2 * M_PI) * stdDev);
	const double exponentConst = 1./(2 * stdDev * stdDev);
	float sum = 0;
	for (int i = 0; i < gaussRad; ++i)
	{
		blurData.coeffs[i].x = NumCast<float>(exp(-i * i * exponentConst) * constantFactor);
		sum += blurData.coeffs[i].x * (1 + (i > 0));
	}
	for (int i = 0; i < gaussRad; ++i)
	{
		blurData.coeffs[i] /= sum;
	}
	blurData.radius = gaussRad - 1;

}

}
