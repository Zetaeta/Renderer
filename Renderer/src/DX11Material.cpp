#include "DX11Material.h"



void DX11TexturedMaterial::Bind(DX11Ctx& ctx)
{
	DX11Material::Bind(ctx);
	ID3D11ShaderResourceView* srvs[] = { m_Albedo->GetSRV(), m_Normal != nullptr ? m_Normal->GetSRV() : nullptr, m_Emissive != nullptr ? m_Emissive->GetSRV() : nullptr };
	ctx.pixelSRVs.Push(srvs);
	ctx.pixelSRVs.Bind(ctx);
//	pContext->PSSetShaderResources(0, u32(std::size(srvs)), srvs);
}

void DX11TexturedMaterial::UnBind(DX11Ctx& ctx)
{
	ctx.pixelSRVs.Pop(3);
}

