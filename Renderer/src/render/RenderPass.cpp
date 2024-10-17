#include "RenderPass.h"
#include "RenderContext.h"

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

}
