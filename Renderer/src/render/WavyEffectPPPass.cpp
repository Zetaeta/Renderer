#include "WavyEffectPPPass.h"
#include "RenderContext.h"
#include "shaders/ShaderDeclarations.h"

namespace rnd
{

struct WavyCB
{
	float Time;
	float Waviness;
};

static auto startTime = std::chrono::system_clock::now();

WavyEffectPPPass::WavyEffectPPPass(RenderContext* rCtx, RGRenderTargetRef dest, RGShaderResources&& resources)
	:PostProcessPass(rCtx, rCtx->GetShader<WavyPPPS>(), dest, std::move(resources), "HeatHaze")
{
}


void WavyEffectPPPass::Execute(IRenderDeviceCtx& deviceCtx)
{
	u64 time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
	float timeMod1 = float(time % 1000) / 1000;
	mCB = deviceCtx.CBPool->AcquireConstantBuffer(ECBLifetime::Dynamic, WavyCB{timeMod1, 3.f});
	deviceCtx.SetConstantBuffers(EShaderType::Pixel, Single<CBHandle const>(mCB));
	PostProcessPass::Execute(deviceCtx);
	deviceCtx.CBPool->ReleaseConstantBuffer(mCB);
}

}
