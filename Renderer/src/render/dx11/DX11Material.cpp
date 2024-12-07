#include "render/dx11/DX11Material.h"
#include "DX11Renderer.h"

namespace rnd
{
namespace dx11
{

void DX11TexturedMaterial::Bind(DX11Ctx& ctx, EShadingLayer layer)
{
	Archetype->Bind(*ctx.mRCtx, layer);
	if (layer == EShadingLayer::NONE || layer == EShadingLayer::Depth)
	{
		ctx.psTextures.ClearFlags();
		ctx.psTextures.Bind(ctx);
		return;
	}
	//ID3D11ShaderResourceView* srvs[] = { m_Albedo->GetSRV(), m_Normal != nullptr ? m_Normal->GetSRV() : nullptr, m_Emissive != nullptr ? m_Emissive->GetSRV() : nullptr };
	ctx.psTextures.SetTexture(E_TS_DIFFUSE, m_Albedo->GetSRV());
	ctx.psTextures.SetTexture(E_TS_ROUGHNESS, GetSRV(m_Roughness));
	ctx.psTextures.SetTexture(E_TS_NORMAL, GetSRV(m_Normal));
	ctx.psTextures.SetTexture(E_TS_EMISSIVE, GetSRV(m_Emissive));
	ctx.psTextures.ClearFlags();
	if (layer < EShadingLayer::SHADING_COUNT)
	{
		ctx.psTextures.SetFlags(F_TS_DIFFUSE);
	}
	if (layer == EShadingLayer::BASE)
	{
		ctx.psTextures.SetFlags(F_TS_EMISSIVE);
	}
	else
	{
		ctx.psTextures.SetFlags(F_TS_NORMAL);
		ctx.psTextures.SetFlags(F_TS_ROUGHNESS);
		//if (layer == EShadingLayer::SPOTLIGHT || layer == EShadingLayer::DIRLIGHT)
		{
			ctx.psTextures.SetFlags(F_TS_SHADOWMAP);
		}
	}
	ctx.psTextures.Bind(ctx);
	//	pContext->PSSetShaderResources(0, u32(std::size(srvs)), srvs);
}

void DX11TexturedMaterial::UnBind(DX11Ctx& ctx)
{
}

void DX11Material::Bind(DX11Ctx& ctx, EShadingLayer layer)
{
	Archetype->Bind(*ctx.mRCtx, layer);
	ctx.psTextures.ClearFlags();
	if (layer == EShadingLayer::SPOTLIGHT || layer == EShadingLayer::DIRLIGHT || layer == EShadingLayer::POINTLIGHT)
	{
		ctx.psTextures.SetFlags(F_TS_SHADOWMAP);
	}
	ctx.psTextures.Bind(ctx);
}

void DX11Material::Bind(rnd::RenderContext& rctx, EShadingLayer layer)
{
	Bind(static_cast<DX11Renderer*>(rctx.DeviceCtx())->m_Ctx, layer);
}

void DX11MaterialType::Bind(rnd::RenderContext& rctx, EShadingLayer layer)
{
	Bind(static_cast<DX11Renderer*>(rctx.DeviceCtx())->m_Ctx, layer);
}

void DX11MaterialType::Bind(DX11Ctx& ctx, EShadingLayer layer)
{
	ctx.pContext->VSSetShader(m_VertexShader.Get(), nullptr, 0);
	ctx.pContext->IASetInputLayout(m_InputLayout.Get());

	auto& pixelShader = PixelShaders[layer >= EShadingLayer::NONE ? EShadingLayer::BASE : layer];
	if (pixelShader == nullptr && Desc.mVSRegistryId != 0)
	{
		pixelShader = Desc.GetShader(ctx.mRCtx->GetShaderManager(), layer, EMatType::E_MT_TRANSL); // TODO: Proper opacity separation
		CBData[ECBFrequency::PS_PerFrame].IsUsed = true; // TEMP fix
		CBData[ECBFrequency::PS_PerInstance].IsUsed = true;
	}
	if (pixelShader && pixelShader->GetDeviceShader())
	{
		ctx.pContext->PSSetShader(static_cast<DX11PixelShader*>(pixelShader->GetDeviceShader())->GetShader(), nullptr, 0);
	}
	else
	{
		ctx.pContext->PSSetShader(m_PixelShader[layer >= EShadingLayer::NONE ? 0 : Denum(layer)].Get(), nullptr, 0);
	}

}

}
}
