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
	CreateResources(textureData);
	printf("%p\n", m_Texture.Get());
}

DX11Texture::DX11Texture(DX11Ctx& ctx, DeviceTextureDesc const& desc, ID3D11Texture2D* texture)
	: DX11TextureBase(ctx, desc)
{
	m_Texture = texture;
	if (desc.Flags & TF_RenderTarget)
	{
		D3D11_TEXTURE2D_DESC dx11Desc;
		texture->GetDesc(&dx11Desc);
		if (desc.Flags & TF_SRGB)
		{
			dx11Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		CreateRenderTarget(dx11Desc);
	}
	if (desc.Flags & TF_SRV)
	{
		D3D11_TEXTURE2D_DESC dx11Desc;
		texture->GetDesc(&dx11Desc);
		CreateSRV(dx11Desc);
	}
	printf("%p\n", texture);
}

DX11Texture::Ref DX11Texture::CreateFrom(TextureRef tex, DX11Ctx* ctx, ETextureFlags flags /*= TF_SRV*/)
{
	u32 mips = 0;
	if (flags & TF_DEPTH) mips = 1;
	DeviceTextureDesc desc;
	desc.Flags = flags;
	desc.Width = tex->width;
	desc.Height = tex->height;
	desc.NumMips = mips;
	desc.DebugName = tex->GetName();
	desc.Format = ETextureFormat::RGBA8_Unorm;
	auto			result = std::make_shared<DX11Texture>(*ctx, desc, tex->GetData());
	//		result->Init(width, height, data, flags);
	return result;
}

void DX11Texture::CreateResources(TextureData textureData /*= {}*/)
{
	auto& desc = Desc;
	auto& ctx = *m_Ctx;
	u32 width = desc.Width, height = desc.Height;
	u32 const *data = reinterpret_cast<u32 const*>(textureData);
	ETextureFlags flags = desc.Flags;

	bool genMips = desc.NumMips == 0;

	auto pDevice = m_Ctx->pDevice;
	bool sRGB = flags & TF_SRGB;
	bool makeSRV = true;
//	sRGB = true;
	// Create texture
	D3D11_TEXTURE2D_DESC textureDesc;
	Zero(textureDesc);
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels =  desc.NumMips;
	textureDesc.ArraySize = 1;
	//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.SampleDesc.Count = desc.SampleCount;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = makeSRV ? D3D11_BIND_SHADER_RESOURCE : 0;
	textureDesc.CPUAccessFlags = 0;

	bool makeRT = desc.Flags & TF_RenderTarget;
	
	if (makeRT)
	{
		textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	textureDesc.Format = GetDxgiFormat(desc.Format, EDxgiFormatContext::Resource);
	//switch (desc.Format)
	//{
	//case ETextureFormat::RGBA8_Unorm:
	//	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//	break;
	//case ETextureFormat::R32_Uint:
	//	textureDesc.Format = DXGI_FORMAT_R32_UINT;
	//	break;
	//case ETextureFormat::D24_Unorm_S8_Uint:
	//	textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	//	break;
	//case ETextureFormat::D32_Float:
	//	textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	//	break;
	//default:
	//	break;
	//}

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
//		textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
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

	if (desc.Flags & TF_SRV)
	{
		// Create texture view
		CreateSRV(textureDesc);
	}
	if (depth && (flags & TF_StencilSRV))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = Desc.SampleCount > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = genMips ? -1 : textureDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		
		if (depth)
		{
			srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		}

		HR_ERR_CHECK(pDevice->CreateShaderResourceView(m_Texture.Get(), &srvDesc, &m_StencilSRV));
		SetResourceName(m_StencilSRV, desc.DebugName + " Stencil SRV");
	}


	if (genMips)
	{
		if (data)
		{
			ctx.pContext->UpdateSubresource(m_Texture.Get(), 0, nullptr, data, subResource.SysMemPitch, subResource.SysMemSlicePitch);
		}
		ctx.pContext->GenerateMips(m_SRV.Get());
	}

	if (desc.Flags & TF_CpuReadback)
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
		switch (desc.Format)
		{
		case ETextureFormat::D24_Unorm_S8_Uint:
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		case ETextureFormat::D32_Float:
			depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
			break;

		default:
			break;
		}
		depthStencilViewDesc.ViewDimension = Desc.SampleCount > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		ComPtr<ID3D11DepthStencilView> DSV;
		HR_ERR_CHECK(pDevice->CreateDepthStencilView(m_Texture.Get(), &depthStencilViewDesc, &DSV));
		if (!Desc.DebugName.empty())
		{
			DSV->SetPrivateData(WKPDID_D3DDebugObjectName, NumCast<u32>(Desc.DebugName.size()), desc.DebugName.c_str());
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
		CreateRenderTarget(textureDesc);
	}
	m_Ctx->m_Renderer->RegisterTexture(this);
}

void DX11Texture::CreateRenderTarget(D3D11_TEXTURE2D_DESC const& textureDesc)
{
	CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(Desc.SampleCount > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D, textureDesc.Format);
	RenderTargetDesc myRtDesc;
	myRtDesc.DebugName = Desc.DebugName + " RT";
	myRtDesc.Dimension = ETextureDimension::TEX_2D;
	myRtDesc.Width = Desc.Width;
	myRtDesc.Height = Desc.Height;
	ComPtr<ID3D11RenderTargetView> RT;
	HR_ERR_CHECK(m_Ctx->pDevice->CreateRenderTargetView(m_Texture.Get(), &rtDesc, &RT));
	mRenderTarget = std::make_shared<DX11RenderTarget>(myRtDesc, RT);
}

void DX11Texture::CreateSRV(D3D11_TEXTURE2D_DESC const& textureDesc)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = textureDesc.Format;
	if (srvDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM && (Desc.Flags & TF_SRGB))
	{
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = (Desc.NumMips == 0) ? -1 : textureDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	
	switch (Desc.Format)
	{
	case ETextureFormat::D24_Unorm_S8_Uint:
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case ETextureFormat::D32_Float:
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	default:
		break;
	}
	

	HR_ERR_CHECK(m_Ctx->pDevice->CreateShaderResourceView(m_Texture.Get(), &srvDesc, &m_SRV));
	SetResourceName(m_SRV, Desc.DebugName);
}

void DX11Texture::CreateSRV()
{
	Desc.Flags &= TF_SRV;
	D3D11_TEXTURE2D_DESC dx11Desc;
	m_Texture->GetDesc(&dx11Desc);
	CreateSRV(dx11Desc);
}

void DX11Texture::DestroyResources()
{
	mRenderTarget = nullptr;
	m_SRV = m_StencilSRV = nullptr;
	m_Texture = m_CpuTexture = nullptr;
}

void* DX11Texture::GetShaderResource(ShaderResourceId id)
{
	switch (id)
	{
	case SRV_StencilBuffer:
		return m_StencilSRV.Get();
	case SRV_Texture:
		return m_SRV.Get();
	default:
		ZE_ENSURE(false);
		return nullptr;
	}
}

void DX11Texture::Resize(u32 width, u32 height)
{
	if (width != Desc.Width || height != Desc.Height)
	{
		Desc.Width = width;
		Desc.Height = height;
		CreateResources();
	}
}

DX11Texture::~DX11Texture()
{
	//if (m_Texture)
	//{
	//	m_Texture->AddRef();
	//	ZE_ASSERT (m_Texture->Release() == 1);
	//}
	if (mDepthStencil)
	{
//		ZE_ASSERT (mDepthStencil.use_count() == 1);
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
	//m_Texture = DX11Texture::Create(&ctx, size, size, nullptr, TF_DEPTH);
} 

}
}
