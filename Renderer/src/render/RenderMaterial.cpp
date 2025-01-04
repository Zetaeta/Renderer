#include "RenderMaterial.h"
#include "RenderTextureManager.h"

namespace rnd
{

void TexturedRenderMaterial::Bind(RenderContext& rctx, EShadingLayer layer)
{
	Archetype->Bind(rctx, layer, mMatType);

	auto& texMgr = rctx.TextureManager;
	if (layer == EShadingLayer::NONE || layer == EShadingLayer::Depth)
	{
		texMgr.ClearFlags();
		texMgr.Bind(rctx.DeviceCtx());
		return;
	}
	//ID3D11ShaderResourceView* srvs[] = { m_Albedo->GetSRV(), m_Normal != nullptr ? m_Normal->GetSRV() : nullptr, m_Emissive != nullptr ? m_Emissive->GetSRV() : nullptr };
	texMgr.SetTexture(E_TS_DIFFUSE, m_Albedo);
	texMgr.SetTexture(E_TS_ROUGHNESS, m_Roughness);
	texMgr.SetTexture(E_TS_NORMAL, m_Normal);
	texMgr.SetTexture(E_TS_EMISSIVE, m_Emissive);
	texMgr.ClearFlags();
	if (layer < EShadingLayer::SHADING_COUNT)
	{
		texMgr.SetFlags(F_TS_DIFFUSE);
	}
	if (layer == EShadingLayer::BASE)
	{
		texMgr.SetFlags(F_TS_EMISSIVE);
	}
	else
	{
		texMgr.SetFlags(F_TS_NORMAL);
		texMgr.SetFlags(F_TS_ROUGHNESS);
		//if (layer == EShadingLayer::SPOTLIGHT || layer == EShadingLayer::DIRLIGHT)
		{
			texMgr.SetFlags(F_TS_SHADOWMAP);
		}
	}
	texMgr.Bind(rctx.DeviceCtx());
	//	pContext->PSSetShaderResources(0, u32(std::size(srvs)), srvs);
}

void RenderMaterial::Bind(RenderContext& rctx, EShadingLayer layer)
{
	Archetype->Bind(rctx, layer, mMatType);
	rctx.TextureManager.ClearFlags();
	if (layer == EShadingLayer::SPOTLIGHT || layer == EShadingLayer::DIRLIGHT || layer == EShadingLayer::POINTLIGHT)
	{
		rctx.TextureManager.SetFlags(F_TS_SHADOWMAP);
	}
	rctx.TextureManager.Bind(rctx.DeviceCtx());
}

}
