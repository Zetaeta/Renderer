#include "RenderGraph.h"

namespace rnd
{

IRenderTarget::Ref RGFlipFlop::GetRenderTarget(RGResourceHandle instanceHandle)
{
	CurrentRead = 1-CurrentRead;
	return Textures[CurrentRead]->GetRT();
}

ResourceView RGFlipFlop::GetResourceView(RGResourceHandle instanceHandle)
{
	return {Textures[CurrentRead], SRV_Texture};
}

RGResourceHandle RGBuilder::MakeFlipFlop(DeviceTextureRef tex1, DeviceTextureRef tex2)
{
	mFlipFlops.emplace_back(tex1, tex2);
	return RGResourceHandle{NumCast<u32>(mFlipFlops.size() - 1), ERGResourceBevhiour::FlipFlop};
}

void RGBuilder::Reset()
{
	for (RGFlipFlop& flipFlop : mFlipFlops)
	{
		flipFlop.Reset();
	}
}

}
