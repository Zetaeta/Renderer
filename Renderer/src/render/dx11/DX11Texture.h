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

class DX11Texture : public DX11TextureBase
{
public:
	DX11Texture(DX11Ctx& ctx, DeviceTextureDesc const& desc, TextureData textureData);

	RCOPY_MOVE_PROTECT(DX11Texture);

	using Ref = std::shared_ptr<DX11Texture>;

	//template<template<typename T> typename TPtr>
	static Ref Create(DX11Ctx* ctx, u32 width, u32 height, u32 const* data, ETextureFlags flags = TF_NONE);
	static Ref CreateFrom(TextureRef tex, DX11Ctx* ctx, ETextureFlags flags = TF_NONE)
	{
		return Create(ctx, u32(tex->width), u32(tex->height), tex->GetData(), flags);
	}

	void* GetHandle()
	{
		return m_SRV.Get();
	}

	void* GetTextureHandle() const override { return GetSRV(); }

	ID3D11ShaderResourceView* GetSRV() const
	{
		return m_SRV.Get();
	}

	ID3D11DepthStencilView* GetDSV()
	{
		return mDepthStencil->GetDSV();
	}
	
	void Init(u32 width, u32 height, u32 const* data, ETextureFlags flags);

	~DX11Texture();

	DX11DepthStencil::Ref mDepthStencil;

	 IRenderTarget::Ref GetRT() override;
	 IDepthStencil::Ref GetDS() override;

 private:
	ComPtr<ID3D11ShaderResourceView> m_SRV = nullptr;
	ComPtr<ID3D11Texture2D> m_Texture;

	std::optional<DX11RenderTarget> m_RT;
//	std::optional<DX11DepthStencil> m_DS;

};

inline ID3D11ShaderResourceView* GetSRV(DX11Texture::Ref const& ref)
{
	return ref == nullptr ? nullptr : ref->GetSRV();
}

class DX11ShadowMap
{
public:
//	DX11ShadowMap(DX11Ctx const& ctx);

	void Init(DX11Ctx& ctx, u32 size);

	auto GetDSV()
	{
		return m_Texture->GetDSV();
	}

	auto GetSRV()
	{
		return m_Texture->GetSRV();
	}
	
	DX11Texture::Ref m_Texture;

	operator bool() {return m_Texture != nullptr;}

	mat4 m_World2LightProj;
};

}
}
