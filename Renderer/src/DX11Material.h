#pragma once

#include "WinUtils.h"
#include <d3d11.h>
#include "ImageDX11.h"
#include "DX11Ctx.h"

enum class EShadingLayer : u8
{
	BASE = 0,
	DIRLIGHT,
	POINTLIGHT,
	SPOTLIGHT,
	SHADING_COUNT,
	NONE = SHADING_COUNT,
	COUNT
};

struct DX11MaterialType
{
	ComPtr<ID3D11PixelShader>  m_PixelShader[Denum(EShadingLayer::COUNT)];
	ComPtr<ID3D11VertexShader> m_VertexShader;
	ComPtr<ID3D11InputLayout> m_InputLayout;
	
	using Ref = DX11MaterialType*;
	virtual void Bind(DX11Ctx& ctx, EShadingLayer layer)
	{
		ctx.pContext->PSSetShader(m_PixelShader[Denum(layer)].Get(), nullptr, 0);
		ctx.pContext->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		ctx.pContext->IASetInputLayout(m_InputLayout.Get());
	}
};



class DX11Material
{
public:
	DX11Material(DX11MaterialType::Ref matType)
		: m_MatType(matType) {}

	DX11MaterialType::Ref m_MatType;

	virtual void Bind(DX11Ctx& ctx, EShadingLayer layer);

	virtual void UnBind(DX11Ctx& ctx)
	{
	}
};

class DX11TexturedMaterial : public DX11Material
{
public:

	DX11TexturedMaterial(DX11MaterialType::Ref matType, std::shared_ptr<ImageDX11> const& albedo)
		: DX11Material(matType), m_Albedo(albedo) {}

	
	virtual void Bind(DX11Ctx& ctx, EShadingLayer layer) override;
	void		 UnBind(DX11Ctx& ctx) override;

	std::shared_ptr<ImageDX11> m_Albedo;
	std::shared_ptr<ImageDX11> m_Normal;
	std::shared_ptr<ImageDX11> m_Emissive;
};

