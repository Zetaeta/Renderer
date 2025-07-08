#pragma once
#include "render/dx11/DX11Ctx.h"
#include "render/DeviceTexture.h"

namespace rnd
{
namespace dx11
{
class DX11TextureBase : public IDeviceTexture
{
public:
	DX11TextureBase(DX11Ctx& ctx, DeviceTextureDesc const& desc);

	ID3D11UnorderedAccessView* GetUAV()
	{
		return mUAV.Get();
	}

protected:
	OpaqueData<8> GetUAV(UavId id) override;
	DX11Ctx* m_Ctx;
	ComPtr<ID3D11UnorderedAccessView> mUAV;
};
}
}
