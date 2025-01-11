#include "render/dx11/DX11Material.h"
#include "DX11Renderer.h"
#include "render/RenderTextureManager.h"

namespace rnd
{
namespace dx11
{

void DX11MaterialType::Bind(rnd::RenderContext& rctx, EShadingLayer layer, EMatType matType)
{
	Bind(static_cast<DX11Renderer*>(rctx.DeviceCtx())->m_Ctx, layer, matType);
}

void DX11MaterialType::Bind(DX11Ctx& ctx, EShadingLayer layer, EMatType matType)
{
	if (mVertexShader == nullptr && Desc.mVSRegistryId)
	{
		OwningPtr<IShaderReflector> reflector;
		mVertexShader = Desc.GetVertexShader(ctx.mRCtx->GetShaderManager(), &reflector);
		ctx.m_Renderer->SetVertexShader(mVertexShader);
		ctx.m_Renderer->SetVertexLayout(GetVertAttHdl<Vertex>());
	}
//	else
	{
		ctx.pContext->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		ctx.pContext->IASetInputLayout(m_InputLayout.Get());
	}

	auto& pixelShader = PixelShaders[layer >= EShadingLayer::NONE ? EShadingLayer::BASE : layer];
	if (pixelShader == nullptr && Desc.mPSRegistryId != 0)
	{
		OwningPtr<IShaderReflector> reflector;
		pixelShader = Desc.GetShader(ctx.mRCtx->GetShaderManager(), layer, matType, mGotPSCBData ? nullptr : &reflector);
					CBData[ECBFrequency::PS_PerInstance].IsUsed = true;
					CBData[ECBFrequency::PS_PerFrame].IsUsed = true;
		//if (reflector)
		//{
		//	for (auto& cb : reflector->GetConstantBuffers())
		//	{
		//		if (FindIgnoreCase(cb.Name, "instance"))
		//		{
		//			CBData[ECBFrequency::PS_PerInstance].IsUsed = true;
		//		}
		//		else if (FindIgnoreCase(cb.Name, "frame"))
		//		{
		//			CBData[ECBFrequency::PS_PerFrame].IsUsed = true;
		//		}
		//	}
		//	mGotPSCBData = true;
		//}
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
