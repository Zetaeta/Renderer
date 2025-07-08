#pragma once
#include "RndFwd.h"
#include "RenderEnums.h"
#include "RenderDevice.h"
#include "RenderDeviceCtx.h"

namespace rnd
{

template<typename T>
void SetConstantBuffer(IRenderDeviceCtx& ctx, EShaderType stage, const T& value, u32 index = 0)
{
	SetConstantBuffer(ctx, stage, reinterpret_cast<u8 const*>(&value), sizeof(value), index);
}


inline void SetConstantBuffer(IRenderDeviceCtx& ctx, EShaderType stage, u8 const* data, u32 dataSize, u32 index = 0)
{
	auto cb = ctx.CBPool->AcquireConstantBuffer(ECBLifetime::Dynamic, Span<u8 const>(data, dataSize));
	ctx.SetConstantBuffers(stage, Single<CBHandle const>(cb));
}

}
