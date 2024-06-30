#include "render/dx11/DX11Texture.h"
#include "core/WinUtils.h"
#include "render/dx11/DX11Ctx.h"
#include "render/dx11/DX11Renderer.h"

 DX11Texture::DX11Texture(DX11Ctx& ctx, DeviceTextureDesc const& desc, TextureData textureData)
	: DX11TextureBase(ctx, desc)
{
	u32 width = desc.width, height = desc.height;
	u32 const *data = reinterpret_cast<u32 const*>(textureData);
	ETextureFlags flags = desc.flags;

	bool genMips = desc.numMips == 0;

	auto pDevice = m_Ctx->pDevice;
	bool sRGB = flags & TF_SRGB;
//	sRGB = true;
	// Create texture
	D3D11_TEXTURE2D_DESC textureDesc;
	Zero(textureDesc);
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels =  desc.numMips;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;

	if (genMips)
	{
		textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	bool depth = flags & TF_DEPTH;
	if (sRGB)
	{
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else if (depth)
	{
		textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}

	D3D11_SUBRESOURCE_DATA subResource;
	Zero(subResource);
	if (data != nullptr)
	{
		subResource.pSysMem = data;
		subResource.SysMemPitch = textureDesc.Width * 4;
		subResource.SysMemSlicePitch = 0;
	}
	HR_ERR_CHECK(pDevice->CreateTexture2D(&textureDesc, (!genMips && subResource.pSysMem) ? &subResource : nullptr, &m_Texture));

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = genMips ? -1 : textureDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	
	if (depth)
	{
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}

	HR_ERR_CHECK(pDevice->CreateShaderResourceView(m_Texture.Get(), &srvDesc, &m_SRV));

	if (genMips)
	{
		if (data)
		{
			ctx.pContext->UpdateSubresource(m_Texture.Get(), 0, nullptr, data, subResource.SysMemPitch, subResource.SysMemSlicePitch);
		}
		ctx.pContext->GenerateMips(m_SRV.Get());
	}

	if (depth)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		ComPtr<ID3D11DepthStencilView> DSV;
		HR_ERR_CHECK(pDevice->CreateDepthStencilView(m_Texture.Get(), &depthStencilViewDesc, &DSV));
		DepthStencilDesc desc
		{
			.Dimension = ETextureDimension::TEX_2D,
			.Width = width,
			.Height = height,
		};
		mDepthStencil = std::make_shared<DX11DepthStencil>(desc, DSV);
	}

	m_Ctx->m_Renderer->RegisterTexture(this);

}

DX11Texture::Ref DX11Texture::Create(DX11Ctx* ctx, u32 width, u32 height, u32 const* data, ETextureFlags flags /*= TF_NONE*/)
{
	u32 mips = 0;
	if (flags & TF_DEPTH) mips = 1;
	DeviceTextureDesc desc{ "", flags, width, height, mips };
	auto			result = std::make_shared<DX11Texture>(*ctx, desc, data);
	//		result->Init(width, height, data, flags);
	return result;
}

DX11Texture::~DX11Texture()
{
	m_Ctx->m_Renderer->UnregisterTexture(this);
}

IRenderTarget::Ref DX11Texture::GetRT()
{
	return nullptr;
}

IDepthStencil::Ref DX11Texture::GetDS()
{
	return mDepthStencil;
}

//DX11ShadowMap::DX11ShadowMap(DX11Ctx const& ctx)
//{
//	D3D11_TEXTURE2D_DESC shadowMapDesc;
//	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
//	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
//	shadowMapDesc.MipLevels = 1;
//	shadowMapDesc.ArraySize = 1;
//	shadowMapDesc.SampleDesc.Count = 1;
//	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
//	shadowMapDesc.Width = 1024;
//	shadowMapDesc.Height = 1024;
//
//	HR_ERR_CHECK(ctx.pDevice->CreateTexture2D(&shadowMapDesc, nullptr, &m_Texture));
//
//	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
//	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
//	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
//	depthStencilViewDesc.Texture2D.MipSlice = 0;
//
//	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
//	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
//	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
//	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
//	shaderResourceViewDesc.Texture2D.MipLevels = 1;
//
//	HR_ERR_CHECK(ctx.pDevice->CreateDepthStencilView(m_Texture.Get(), &depthStencilViewDesc, &m_DepthView));
//	HR_ERR_CHECK(ctx.pDevice->CreateShaderResourceView(m_Texture.Get(), &shaderResourceViewDesc, &m_SRV));
//}

void DX11ShadowMap::Init(DX11Ctx& ctx, u32 size)
{
	m_Texture = DX11Texture::Create(&ctx, size, size, nullptr, TF_DEPTH);
} 

