#pragma once
#include "passes/PostProcessPass.h"

namespace rnd
{

class WavyEffectPPPass : public PostProcessPass
{
public:
	WavyEffectPPPass(RenderContext* rCtx, RGRenderTargetRef dest, RGShaderResources&& resources);
	virtual void Execute(RenderContext& renderCtx) override;

	PooledCBHandle mCB;
};

}