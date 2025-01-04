#pragma once
#include <render/DeviceTexture.h>
#include "SharedD3D11.h"

namespace rnd
{
namespace dx11
{

class DX11RenderTarget : public IRenderTarget
{
public:
	DX11RenderTarget(RenderTargetDesc const& desc, ComPtr<ID3D11RenderTargetView> rt);

	OpaqueData<8> GetRTData() override { return RenderTargets[0].Get(); }
	OpaqueData<8> GetData() const override { return RenderTargets[0].Get(); }

	ID3D11RenderTargetView* GetRTV() const { return RenderTargets[0].Get(); }
	ID3D11RenderTargetView* GetRTV(u32 idx) const { return RenderTargets[idx].Get(); }

	Vector<ComPtr<ID3D11RenderTargetView>> RenderTargets;
//	ComPtr<ID3D11DepthStencilView> DepthStencil;

};

class DX11DepthStencil : public IDepthStencil
{
public:
	DX11DepthStencil(DepthStencilDesc desc)
		:IDepthStencil(desc)
	{}
	DX11DepthStencil(DepthStencilDesc desc, ComPtr<ID3D11DepthStencilView> dsv)
		:IDepthStencil(desc), DepthStencils({dsv})
	{}

	DX11DepthStencil(DepthStencilDesc desc, Vector<ComPtr<ID3D11DepthStencilView>>&& dsvs)
		:IDepthStencil(desc), DepthStencils(std::move(dsvs))
	{}
	~DX11DepthStencil();

	OpaqueData<8> GetData() const override { return DepthStencils[0].Get(); }

	ID3D11DepthStencilView* GetDSV() const
	{
		ZE_ASSERT(Desc.Dimension == ETextureDimension::TEX_2D);
		return DepthStencils[0].Get();
	}
	ID3D11DepthStencilView* GetDSV(u32 idx) const
	{
		ZE_ASSERT(Desc.Dimension != ETextureDimension::TEX_2D);
		return DepthStencils[idx].Get();
	}

	Vector<ComPtr<ID3D11DepthStencilView>> DepthStencils;

	using Ref = std::shared_ptr<DX11DepthStencil>;

};

}
}
