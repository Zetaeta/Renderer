#pragma once

#include "render/SceneRenderPass.h"
#include "PostProcessPass.h"
#include "render/RenderGraph.h"

namespace rnd
{

//#define PASS_PARAMETER()
using SRVType = RGResourceHandle;

class DeferredShadingPass : public RenderPass
{
public:
	DeferredShadingPass(RenderContext* rCtx, Camera::Ref camera, IRenderTarget::Ref dest, SRVType sceneColour,
						SRVType sceneNormal, SRVType sceneEmissive, SRVType sceneDepth, SRVType ambientOcclusion);

	void Execute(IRenderDeviceCtx& deviceCtx) override;
	void Build(RGBuilder& builder) override;
private:
	std::array<RefPtr<const PixelShader>, Denum(EShadingLayer::SHADING_COUNT)> mPixelShader;
	RefPtr<const VertexShader> mVertexShader;
	//SRVType mSceneColour;
	//SRVType mSceneNormal;
	//SRVType mSceneEmissive;
	//SRVType mSceneDepth;

	RGShaderResources mSRVs;
	IRenderTarget::Ref mRenderTarget;
	Camera::Ref mCamera;
};

}
