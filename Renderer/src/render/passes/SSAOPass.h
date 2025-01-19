#pragma once
#include "render/RenderPass.h"
#include "render/RenderGraph.h"

namespace rnd
{


class SSAOPass : public RenderPass
{
public:
	SSAOPass(RenderContext* rCtx, RGShaderResources&& srvs, RGUnorderedAccessViews&& uavs);
	~SSAOPass();
	void Execute(RenderContext& renderCtx) override;
	void Build(RGBuilder& builder) override;

	void AddControls() override;

	void Rerandomize();
	void CalcGaussian();

private:
	float mThreshold = 0.3f;
	float mRadius = 0.3f;
	int normalType = 0;
	int mNumSamples = 16;
	int gaussRad = 6;
	bool hemisphere = true;
	float stdDev = 2.f;
	bool blur = true;
	bool mDebugPixel = false;
	RGShaderResources mSRVs;
	RGResourceHandle mAOTexture;
	RGResourceHandle mSecondaryTexture;
//	RGUnorderedAccessViews mUAVs;
	ResourceView mAOTextureIn;
	ResourceView mSecondaryTextureIn;
	UnorderedAccessView mAOTextureUav;
	UnorderedAccessView mSecondaryTextureUav;
	
	RefPtr<ComputeShader const> mShader;
	RefPtr<ComputeShader const> mDebugShader;
	RefPtr<ComputeShader const> mDownsampler;
};

}
