#pragma once
#include "asset/Texture.h"
#include <d3d11.h>
#include "core/WinUtils.h"
#include <render/dx11/DX11TextureBase.h>
#include "DX11RenderTarget.h"

namespace rnd
{
namespace dx11
{
class DX11Cubemap : public DX11TextureBase
{
public:
	DX11Cubemap(DX11Ctx& ctx, DeviceTextureDesc const& desc, D3D11_SUBRESOURCE_DATA const* initData = nullptr);

	virtual ~DX11Cubemap();

	static DX11Cubemap FoldUp(DX11Ctx& ctx, TextureRef tex);

	ID3D11ShaderResourceView* GetSRV() const
	{
		return srv.Get();
	}

	ID3D11DepthStencilView* GetDSV(u32 i)
	{
		return mDepthStencil->DepthStencils[i].Get();
	}

	OpaqueData<8> GetTextureHandle() const { return GetSRV(); }

	OpaqueData<8> GetData() const override
	{
		return nullptr;
	}

	ComPtr<ID3D11ShaderResourceView> srv;
	ComPtr<ID3D11Texture2D> mTexture;
	//Vector<ComPtr<ID3D11DepthStencilView>> m_DSVs;
	DX11DepthStencil::Ref mDepthStencil;


	IRenderTarget::Ref GetRT() override;
	IDepthStencil::Ref GetDS() override;

	OpaqueData<8> GetRHIResource() const override
	{
		return mTexture.Get();
	}


	 MappedResource Map(u32 subResource, ECpuAccessFlags flags) override;


	 void Unmap(u32 subResource) override;


	 void CreateSRV(u32) override
	 {
		 throw std::logic_error("The method or operation is not implemented.");
	 }

	 CopyableMemory<8> GetShaderResource(ShaderResourceId id) override
	 {
		 return srv.Get();
	 }

 private:
	DX11Cubemap(DX11Ctx& ctx);
};
}
}
