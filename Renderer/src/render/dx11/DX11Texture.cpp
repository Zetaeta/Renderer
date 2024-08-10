#include "render/dx11/DX11Texture.h"
#include "core/WinUtils.h"
#include "render/dx11/DX11Ctx.h"
#include "render/dx11/DX11Renderer.h"

namespace rnd
{
namespace dx11
{
 DX11Texture::DX11Texture(DX11Ctx& ctx, DeviceTextureDesc const& desc, TextureData textureData)
	: DX11TextureBase(ctx, desc)
{
	u32 width = desc.width, height = desc.height;
	u32 const *data = reinterpret_cast<u32 const*>(textureData);
	ETextureFlags flags = desc.flags;

	bool genMips = desc.numMips == 0;

	auto pDevice = m_Ctx->pDevice;
	bool sRGB = flags & TF_SRGB;
	bool makeSRV = true;
//	sRGB = true;
	// Create texture
	D3D11_TEXTURE2D_DESC textureDesc;
	Zero(textureDesc);
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels =  desc.numMips;
	textureDesc.ArraySize = 1;
	//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = makeSRV ? D3D11_BIND_SHADER_RESOURCE : 0;
	textureDesc.CPUAccessFlags = 0;

	bool makeRT = desc.flags & TF_RenderTarget;
	
	if (makeRT)
	{
		textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	switch (desc.format)
	{
	case ETextureFormat::RGBA8_Unorm:
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case ETextureFormat::R32_Uint:
		textureDesc.Format = DXGI_FORMAT_R32_UINT;
		break;
	default:
		break;
	}

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
	SetResourceName(m_Texture, desc.DebugName);

//	if (desc.flags & TF_SRV)
	{
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
		SetResourceName(m_SRV, desc.DebugName);
	}


	if (genMips)
	{
		if (data)
		{
			ctx.pContext->UpdateSubresource(m_Texture.Get(), 0, nullptr, data, subResource.SysMemPitch, subResource.SysMemSlicePitch);
		}
		ctx.pContext->GenerateMips(m_SRV.Get());
	}

	if (desc.flags & TF_CpuReadback)
	{
		D3D11_TEXTURE2D_DESC cpuDesc = textureDesc;
		cpuDesc.Usage = D3D11_USAGE_STAGING;
		cpuDesc.BindFlags = 0;
		cpuDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		ctx.pDevice->CreateTexture2D(&cpuDesc, nullptr, &m_CpuTexture);
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
		if (!Desc.DebugName.empty())
		{
			DSV->SetPrivateData(WKPDID_D3DDebugObjectName, Desc.DebugName.size(), desc.DebugName.c_str());
		}

		DepthStencilDesc desc
		{
			.Dimension = ETextureDimension::TEX_2D,
			.Width = width,
			.Height = height,
		};
		mDepthStencil = std::make_shared<DX11DepthStencil>(desc, DSV);
	}

	if (makeRT)
	{
		CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2D, textureDesc.Format);
		RenderTargetDesc myRtDesc;
		myRtDesc.DebugName = desc.DebugName + " RT";
		myRtDesc.Dimension = ETextureDimension::TEX_2D;
		myRtDesc.Width = width;
		myRtDesc.Height = height;
		ComPtr<ID3D11RenderTargetView> RT;
		HR_ERR_CHECK(pDevice->CreateRenderTargetView(m_Texture.Get(), &rtDesc, &RT));
		mRenderTarget = std::make_shared<DX11RenderTarget>(myRtDesc, RT);
	}

	m_Ctx->m_Renderer->RegisterTexture(this);

}

DX11Texture::Ref DX11Texture::Create(DX11Ctx* ctx, u32 width, u32 height, u32 const* data, ETextureFlags flags /*= TF_NONE*/)
{
	u32 mips = 0;
	if (flags & TF_DEPTH) mips = 1;
	DeviceTextureDesc desc{ "", flags, ETextureFormat::RGBA8_Unorm, width, height, mips };
	auto			result = std::make_shared<DX11Texture>(*ctx, desc, data);
	//		result->Init(width, height, data, flags);
	return result;
}

DX11Texture::~DX11Texture()
{
	if (m_Texture)
	{
		m_Texture->AddRef();
		RASSERT(m_Texture->Release() == 1);
	}
	if (mDepthStencil)
	{
//		RASSERT(mDepthStencil.use_count() == 1);
	}

	CHECK_COM_REF(m_CpuTexture);
	CHECK_COM_REF(m_SRV);
	m_Ctx->m_Renderer->UnregisterTexture(this);
}

IRenderTarget::Ref DX11Texture::GetRT()
{
	return mRenderTarget;
}

IDepthStencil::Ref DX11Texture::GetDS()
{
	return mDepthStencil;
}

MappedResource DX11Texture::Map(u32 subResource, ECpuAccessFlags flags)
{
	if (m_CpuTexture)
	{
		m_Ctx->pContext->CopyResource(m_CpuTexture.Get(), m_Texture.Get());
		return m_Ctx->m_Renderer->MapResource(m_CpuTexture.Get(), subResource, flags);
	}
	else
	{
		return m_Ctx->m_Renderer->MapResource(m_Texture.Get(), subResource, flags);
	}
}

void DX11Texture::Unmap(u32 subResource)
{
	if (m_CpuTexture)
	{
		m_Ctx->pContext->Unmap(m_CpuTexture.Get(), subResource);
	}
	else
	{
		m_Ctx->pContext->Unmap(m_Texture.Get(), subResource);
	}
}

void DX11ShadowMap::Init(DX11Ctx& ctx, u32 size)
{
	m_Texture = DX11Texture::Create(&ctx, size, size, nullptr, TF_DEPTH);
} 

}
}
