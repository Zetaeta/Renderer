#include "DX11Cubemap.h"



DX11Cubemap::DX11Cubemap(DX11Ctx& ctx, DX11TextureDesc const& desc, D3D11_SUBRESOURCE_DATA const* initData)
	: DX11TextureBase(ctx, desc)
{
	
	ID3D11Texture2D* cubeTex;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	u32			faceWidth = static_cast<u32>(desc.width);
	// assert(faceWidth * 3 == tex->height);
	u32					 faceHeight = faceWidth;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = faceWidth;
	texDesc.Height = faceHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = format;
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;


	if (desc.flags & TF_DEPTH)
	{
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}

	HR_ERR_CHECK(ctx.pDevice->CreateTexture2D(&texDesc, initData, &cubeTex));
	HR_ERR_CHECK(ctx.pDevice->CreateShaderResourceView(cubeTex, &srvDesc, &srv));
	if (desc.flags & TF_DEPTH)
	{
		for (int i=0; i<6; ++i)
		{
			ID3D11DepthStencilView* dsv;
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.FirstArraySlice = i;
			dsvDesc.Texture2DArray.ArraySize = 1;
			dsvDesc.Texture2DArray.MipSlice = 0;
			HR_ERR_CHECK(ctx.pDevice->CreateDepthStencilView(cubeTex, &dsvDesc, &dsv));
			m_DSVs.emplace_back(dsv);
		}
	}
}

 DX11Cubemap::DX11Cubemap(DX11Ctx& ctx)
	: DX11TextureBase(ctx, DX11TextureDesc{})
{
}

DX11Cubemap DX11Cubemap::FoldUp(DX11Ctx& ctx, TextureRef tex)
{
	ID3D11Device* device = ctx.pDevice;
	DX11Cubemap			res(ctx);
	res.m_Desc.flags = TF_SRGB;
	constexpr u32 const startPos[6] = {
		6, 4, 1, 9, 5, 7

	};
	ID3D11Texture2D* cubeTex;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	u32			faceWidth = static_cast<u32>(tex->width / 4);
	// assert(faceWidth * 3 == tex->height);
	u32					 faceHeight = faceWidth;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = faceWidth;
	texDesc.Height = faceHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = format;
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;
	D3D11_SUBRESOURCE_DATA sData[6];
	u64					   faceSize = faceHeight * faceWidth;
	for (int i = 0; i < 6; ++i)
	{
		sData[i].pSysMem = tex->GetData() + (faceSize * (startPos[i] / 4) * 4) + faceWidth * (startPos[i] % 4);
		sData[i].SysMemPitch = u32(tex->width) * sizeof(u32);
		sData[i].SysMemSlicePitch = 0;
	}
	HR_ERR_CHECK(device->CreateTexture2D(&texDesc, &sData[0], &cubeTex));
	HR_ERR_CHECK(device->CreateShaderResourceView(cubeTex, &srvDesc, &res.srv));

	return res;
}

void DX11ShadowCube::Init(DX11Ctx& ctx, u32 size)
{
	DX11TextureDesc desc;
	desc.flags = TF_DEPTH;
	desc.width = size;
	desc.height = size;
	m_Cube = std::make_unique<DX11Cubemap>(ctx, desc);
}
