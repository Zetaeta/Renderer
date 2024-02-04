#include "CubemapDX11.h"



CubemapDX11 CubemapDX11::FoldUp(ID3D11Device* device, TextureRef tex)
{
	CubemapDX11			res;
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
