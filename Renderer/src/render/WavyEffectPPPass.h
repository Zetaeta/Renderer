#pragma once
#include "passes/PostProcessPass.h"
#include "RenderDevice.h"

namespace rnd
{

class WavyEffectPPPass : public PostProcessPass
{
public:
	WavyEffectPPPass(RenderContext* rCtx, RGRenderTargetRef dest, RGShaderResources&& resources);
	virtual void Execute(IRenderDeviceCtx& deviceCtx) override;

	PooledCBHandle mCB;
};

}
