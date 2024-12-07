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

	void* GetTextureHandle() const { return GetSRV(); }

	void* GetData() const override
	{
		return nullptr;
	}

	ComPtr<ID3D11ShaderResourceView> srv;
	//Vector<ComPtr<ID3D11DepthStencilView>> m_DSVs;
	DX11DepthStencil::Ref mDepthStencil;


	IRenderTarget::Ref GetRT() override;
	IDepthStencil::Ref GetDS() override;


 MappedResource Map(u32 subResource, ECpuAccessFlags flags) override;


 void Unmap(u32 subResource) override;


 void CreateSRV() override
 {
	 throw std::logic_error("The method or operation is not implemented.");
 }


 void* GetShaderResource(ShaderResourceId id) override
 {
	 return srv.Get();
 }

 private:
	DX11Cubemap(DX11Ctx& ctx);
};

class DX11ShadowCube
{
public:
//	DX11ShadowMap(DX11Ctx const& ctx);

	void Init(DX11Ctx& ctx, u32 size);

	auto GetDSV(u32 i)
	{
		return m_Cube->GetDSV(i);
	}

	auto GetSRV()
	{
		return m_Cube->GetSRV();
	}
	
	OwningPtr<DX11Cubemap> m_Cube;

	operator bool() {return m_Cube != nullptr;}

	mat4 m_World2Light;
};
}
}
