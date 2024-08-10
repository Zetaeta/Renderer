#include "RenderPass.h"
#include "RenderContext.h"

namespace rnd
{

IRenderDeviceCtx* RenderPass::DeviceCtx()
{
	return mRCtx->DeviceCtx();
}

}
