#pragma once
#include "Texture.h"
#include <d3d11.h>
#include "WinUtils.h"
#include <dx11/DX11TextureBase.h>

class DX11Cubemap : public DX11TextureBase
{
public:
	DX11Cubemap(DX11Ctx& ctx, DX11TextureDesc const& desc, D3D11_SUBRESOURCE_DATA const* initData = nullptr);


	static DX11Cubemap FoldUp(DX11Ctx& ctx, TextureRef tex);

	ID3D11ShaderResourceView* GetSRV()
	{
		return srv.Get();
	}

	ID3D11DepthStencilView* GetDSV(u32 i)
	{
		return m_DSVs[i].Get();
	}

	ComPtr<ID3D11ShaderResourceView> srv;
	Vector<ComPtr<ID3D11DepthStencilView>> m_DSVs;

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
