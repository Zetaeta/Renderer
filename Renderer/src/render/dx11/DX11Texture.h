#pragma once
#include <d3d11.h>
#include "core/Maths.h"
#include "render/dx11/DX11Backend.h"
#include <wrl.h>
#include <memory>
#include "core/Utils.h"
#include "asset/Texture.h"
#include "DX11TextureBase.h"
#include "DX11RenderTarget.h"

namespace rnd
{
namespace dx11
{

struct DX11Ctx;

using Microsoft::WRL::ComPtr;

class DX11Texture final : public DX11TextureBase
{
public:
	DX11Texture(DX11Ctx& ctx, DeviceTextureDesc const& desc, TextureData textureData);

	// Construct from already created device texture
	DX11Texture(DX11Ctx& ctx, DeviceTextureDesc const& desc, ID3D11Texture2D* texture);

	RCOPY_MOVE_PROTECT(DX11Texture);

	using Ref = std::shared_ptr<DX11Texture>;

	//template<template<typename T> typename TPtr>
	static Ref CreateFrom(TextureRef tex, DX11Ctx* ctx, ETextureFlags flags = TF_SRV);

	void Resize(u32 width, u32 height);

	void* GetHandle()
	{
		return m_SRV.Get();
	}

	OpaqueData<8> GetData() const override
	{
		return m_Texture.Get();
	}
	OpaqueData<8> GetRHIResource() const override
	{
		return m_Texture.Get();
	}

	OpaqueData<8> GetTextureHandle() const override { return GetSRV(); }

	ID3D11ShaderResourceView* GetSRV() const
	{
		return m_SRV.Get();
	}

	ID3D11DepthStencilView* GetDSV()
	{
		return mDepthStencil->GetDSV();
	}

	ID3D11Texture2D* GetTexture()
	{
		return m_Texture.Get();
	}


	void Init(u32 width, u32 height, u32 const* data, ETextureFlags flags);

	~DX11Texture();

	DX11DepthStencil::Ref mDepthStencil;

	IRenderTarget::Ref GetRT() override;
	IDepthStencil::Ref GetDS() override;

	virtual MappedResource Map(u32 subResource, ECpuAccessFlags flags) override;
	virtual void		   Unmap(u32 subResource) override;

public:
	void CreateSRV(u32 subresource = 0) override;
	UavId CreateUAV(u32 subresource = 0);

	void DestroyResources();

 private:
	void CreateResources(TextureData textureData = {});
	void CreateRenderTarget(D3D11_TEXTURE2D_DESC const& textureDesc);
	void CreateSRV(D3D11_TEXTURE2D_DESC const& textureDesc);

	CopyableMemory<8> GetShaderResource(ShaderResourceId id) override;

	ComPtr<ID3D11ShaderResourceView> m_SRV = nullptr;
	ComPtr<ID3D11ShaderResourceView> m_StencilSRV = nullptr;
	ComPtr<ID3D11Texture2D> m_Texture;
	ComPtr<ID3D11Texture2D> m_CpuTexture;

	std::shared_ptr<DX11RenderTarget> mRenderTarget;
//	std::optional<DX11DepthStencil> m_DS;

};

inline ID3D11ShaderResourceView* GetSRV(DX11Texture::Ref const& ref)
{
	return ref == nullptr ? nullptr : ref->GetSRV();
}

}
}
