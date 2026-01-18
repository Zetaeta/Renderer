#include "RenderMaterial.h"
#include "RenderTextureManager.h"
#include "RenderContext.h"
#include "shaders/ShaderReflection.h"
#include "asset/Mesh.h"

namespace rnd
{

void TexturedRenderMaterial::Bind(RenderContext& rctx, EShadingLayer layer)
{
	Archetype->Bind(rctx, layer, mMatType);

	auto& texMgr = rctx.TextureManager;
	//if (layer == EShadingLayer::NONE || layer == EShadingLayer::Depth)
	//{
	//	texMgr.ClearFlags();
	//	texMgr.Bind(rctx.DeviceCtx());
	//	return;
	//}
	texMgr.SetTexture(E_TS_DIFFUSE, m_Albedo);
	texMgr.SetTexture(E_TS_ROUGHNESS, m_Roughness);
	texMgr.SetTexture(E_TS_NORMAL, m_Normal);
	texMgr.SetTexture(E_TS_EMISSIVE, m_Emissive);
	texMgr.ClearFlags();
//	if (layer < EShadingLayer::SHADING_COUNT)
	{
		texMgr.SetFlags(F_TS_DIFFUSE);
	}
	if (layer == EShadingLayer::BASE)
	{
		texMgr.SetFlags(F_TS_EMISSIVE);
	}
	else if (layer < EShadingLayer::SHADING_COUNT)
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

MaterialArchetype::MaterialArchetype(MaterialArchetypeDesc const& desc, IRenderDevice& device)
	: mDevice(&device)
{
	CBData[Denum(ECBFrequency::PS_PerInstance)].Layout = GetLayout<PerInstancePSData>();
	DebugName = desc.DebugName;
	Desc = desc;
}

MaterialArchetype::~MaterialArchetype()
{

}

void MaterialArchetype::Bind(rnd::RenderContext& rctx, EShadingLayer layer, EMatType matType)
{
	ZE_ASSERT(rctx.GetDevice() == mDevice);
	auto ctx = rctx.DeviceCtx();
	if (mVertexShader == nullptr && Desc.mVSRegistryId)
	{
		OwningPtr<IShaderReflector> reflector;
		mVertexShader = Desc.GetVertexShader(rctx.GetShaderManager(), &reflector);
		if (reflector)
		{
			for (auto& cb : reflector->GetConstantBuffers())
			{
				if (FindIgnoreCase(cb.Name.c_str(), "instance"))
				{
					CBData[ECBFrequency::VS_PerInstance].IsUsed = true;
				}
				else if (FindIgnoreCase(cb.Name.c_str(), "frame"))
				{
					CBData[ECBFrequency::VS_PerFrame].IsUsed = true;
				}
			}
		}
		//ctx.m_Renderer->SetVertexShader(nullptr);
		//ctx.m_Renderer->SetVertexLayout(-1);
	}
	ctx->SetVertexShader(mVertexShader);
	ctx->SetVertexLayout(GetVertAttHdl<Vertex>());

	auto& pixelShader = PixelShaders[layer >= EShadingLayer::NONE ? EShadingLayer::BASE : layer];
	if (pixelShader == nullptr && Desc.mPSRegistryId != 0)
	{
		OwningPtr<IShaderReflector> reflector;
		pixelShader = Desc.GetShader(rctx.GetShaderManager(), layer, matType, mGotPSCBData ? nullptr : &reflector);
		if (reflector)
		{
			for (auto& cb : reflector->GetConstantBuffers())
			{
				if (FindIgnoreCase(cb.Name.c_str(), "instance"))
				{
					CBData[ECBFrequency::PS_PerInstance].IsUsed = true;
				}
				else if (FindIgnoreCase(cb.Name.c_str(), "frame"))
				{
					CBData[ECBFrequency::PS_PerFrame].IsUsed = true;
				}
			}
			mGotPSCBData = true;
		}
	}
	if (pixelShader && pixelShader->GetDeviceShader())
	{
		ctx->SetPixelShader(pixelShader);
	}

}

DEFINE_CLASS_TYPEINFO(PerInstancePSData)
BEGIN_REFL_PROPS()
REFL_PROP(colour)
REFL_PROP(ambdiffspec)
REFL_PROP(roughness)
REFL_PROP(emissiveColour)
REFL_PROP(alphaMask)
REFL_PROP(metalness)
REFL_PROP(useNormalMap)
REFL_PROP(useEmissiveMap)
REFL_PROP(useRoughnessMap)
END_REFL_PROPS()
END_CLASS_TYPEINFO()


}
