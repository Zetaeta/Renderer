#include "RenderGraph.h"
#include "RenderDevice.h"

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

RGResourceHandle RGBuilder::AddFixedSRV(ResourceView view)
{
	mFixedSRVs.push_back(RGFixedSRV{ view });
	return RGResourceHandle{ NumCast<u32>(mFixedSRVs.size() - 1), ERGResourceBevhiour::FixedSRV };
}

RGResourceHandle RGBuilder::MakeFlipFlop(DeviceTextureRef tex1, DeviceTextureRef tex2)
{
	mFlipFlops.emplace_back(tex1, tex2);
	return RGResourceHandle{NumCast<u32>(mFlipFlops.size() - 1), ERGResourceBevhiour::FlipFlop};
}

RGResourceHandle RGBuilder::MakeTexture2D(DeviceTextureDesc const& desc)
{
	mTextures.emplace_back(mDevice->CreateTexture2D(desc));
	return RGResourceHandle{NumCast<u32>(mTextures.size() - 1), ERGResourceBevhiour::Texture};
}

void RGBuilder::Reset()
{
	for (RGFlipFlop& flipFlop : mFlipFlops)
	{
		flipFlop.Reset();
	}
}

IRenderTarget::Ref RGTexture::GetRenderTarget(RGResourceHandle instanceHandle)
{
	return mTexture->GetRT();
}

IDepthStencil::Ref RGTexture::GetDSV(RGResourceHandle instanceHandle)
{
	return mTexture->GetDS();
}

ResourceView RGTexture::GetResourceView(RGResourceHandle instanceHandle)
{
	return {mTexture, SRV_Texture};
}

UnorderedAccessView RGTexture::GetUAV(RGResourceHandle instanceHandle)
{
	return {mTexture, instanceHandle.Subresource};
}

}
