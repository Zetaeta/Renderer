#pragma once

#include "core/WinUtils.h"
#include <d3d11.h>
#include "render/dx11/DX11Texture.h"
#include "render/dx11/DX11Ctx.h"
#include "render/RenderMaterial.h"
#include <scene/Lights.h>
#include <common/Material.h>
namespace rnd
{
namespace dx11
{

struct DX11MaterialType : public MaterialArchetype
{
	DX11MaterialType() = default;
	~DX11MaterialType() {}

	DX11MaterialType(DX11MaterialType&&) noexcept = default;
	DX11MaterialType(DX11MaterialType const&) = delete;
	DX11MaterialType& operator=(DX11MaterialType&&) noexcept = default;

	ComPtr<ID3D11PixelShader>  m_PixelShader[Denum(EShadingLayer::Count)];
	ComPtr<ID3D11VertexShader> m_VertexShader;
	ComPtr<ID3D11InputLayout> m_InputLayout;
	std::unique_ptr<class RenderMaterial> m_Default;
	virtual void Bind(rnd::RenderContext& rctx, EShadingLayer layer, EMatType matType) override;
	
	using Ref = RefPtr<DX11MaterialType>;
	virtual void Bind(DX11Ctx& ctx, EShadingLayer layer, EMatType matType);
	bool mGotVSCBData = false;
	bool mGotPSCBData = false;

};

}
}
