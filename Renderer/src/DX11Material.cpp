#include "DX11Material.h"



void DX11TexturedMaterial::Bind(DX11Ctx& ctx, EShadingLayer layer)
{
	m_MatType->Bind(ctx, layer);
	if (layer == EShadingLayer::NONE) {
	
		ctx.psTextures.ClearFlags();
		ctx.psTextures.Bind(ctx);
	}
	//ID3D11ShaderResourceView* srvs[] = { m_Albedo->GetSRV(), m_Normal != nullptr ? m_Normal->GetSRV() : nullptr, m_Emissive != nullptr ? m_Emissive->GetSRV() : nullptr };
	ctx.psTextures.SetTexture(E_TS_DIFFUSE, m_Albedo->GetSRV());
	ctx.psTextures.SetTexture(E_TS_ROUGHNESS, GetSRV(m_Roughness));
	ctx.psTextures.SetTexture(E_TS_NORMAL, GetSRV(m_Normal));
	ctx.psTextures.SetTexture(E_TS_EMISSIVE, GetSRV(m_Emissive));
	ctx.psTextures.ClearFlags();
	ctx.psTextures.SetFlags(F_TS_DIFFUSE);
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
	m_MatType->Bind(ctx, layer);
	ctx.psTextures.ClearFlags();
	if (layer == EShadingLayer::SPOTLIGHT || layer == EShadingLayer::DIRLIGHT || layer == EShadingLayer::POINTLIGHT)
	{
		ctx.psTextures.SetFlags(F_TS_SHADOWMAP);
	}
	ctx.psTextures.Bind(ctx);
}
