#include "RenderPass.h"
#include "RenderContext.h"
#include <locale>
#include <codecvt>

namespace rnd
{

IRenderDeviceCtx* RenderPass::DeviceCtx()
{
	return mRCtx->DeviceCtx();
}

IRenderDevice* RenderPass::Device()
{
	return DeviceCtx()->Device;
}


RenderPass::RenderPass(RenderContext* rCtx, String&& name /*= ""*/) : mRCtx(rCtx), PassName(std::move(name))
{
//	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	wchar_t buffer[64];
	MultiByteToWideChar(CP_ACP, 0, PassName.c_str(), -1, buffer, 64);
	buffer[63] = 0;
	mTimer = DeviceCtx()->CreateTimer(buffer);
}

#if PROFILING
void RenderPass::ExecuteWithProfiling(IRenderDeviceCtx& deviceCtx)
{
	deviceCtx.StartTimer(mTimer);
	Execute(deviceCtx);
	deviceCtx.StopTimer(mTimer);
}
#endif

}
