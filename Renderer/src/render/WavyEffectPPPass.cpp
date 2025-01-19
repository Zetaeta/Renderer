#include "WavyEffectPPPass.h"
#include "RenderContext.h"
#include "shaders/ShaderDeclarations.h"

namespace rnd
{

struct WavyCB
{
	float Time;
};

static auto startTime = std::chrono::system_clock::now();

WavyEffectPPPass::WavyEffectPPPass(RenderContext* rCtx, RGRenderTargetRef dest, RGShaderResources&& resources)
	:PostProcessPass(rCtx, rCtx->GetShader<WavyPPPS>(), dest, std::move(resources), "HeatHaze")
{
	float f = 0;
}


void WavyEffectPPPass::Execute(RenderContext& renderCtx)
{
	u64 time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
	float timeMod1 = float(time % 1000) / 1000;
	mCB = renderCtx.DeviceCtx()->CBPool->AcquireConstantBuffer(ECBLifetime::Dynamic, timeMod1);
	renderCtx.DeviceCtx()->UpdateConstantBuffer(mCB, (float) timeMod1);
	renderCtx.DeviceCtx()->SetConstantBuffers(EShaderType::Pixel, Span<CBHandle>(static_cast<CBHandle*>(&mCB), 1u));
	PostProcessPass::Execute(renderCtx);
	renderCtx.DeviceCtx()->CBPool->ReleaseConstantBuffer(mCB);
}

}
