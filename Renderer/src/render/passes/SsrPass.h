#pragma once
#include "render/RenderPass.h"
#include "render/RenderGraph.h"

namespace rnd
{

class SsrPass : public RenderPass
{
public:
	SsrPass(RenderContext* rCtx, RGShaderResources&& srvs, RGRenderTargetRef renderTarget);
	void Execute(IRenderDeviceCtx& deviceCtx) override;
	void Build(RGBuilder& builder) override;

	void AddControls() override;

protected:
	RGShaderResources mSrvs;
	RGRenderTargetRef mRT;
//	RefPtr<ComputeShader const> mShader;

	float reflectionThreshold = 0.5f;
	float maxDist = 10.f;
	float traceResolution = 0.3f;
	uint maxRefineSteps = 32;
	float hitBias = 0.0f;
	bool mDebugPixel = true;
};

}
