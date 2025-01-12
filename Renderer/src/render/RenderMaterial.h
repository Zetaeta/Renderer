#pragma once
#include "common/Material.h"

namespace rnd
{

using GPUBool = u32;

__declspec(align(16))
struct PerInstancePSData
{
	DECLARE_STI_NOBASE(PerInstancePSData);
	vec4 colour;
	vec3 ambdiffspec;
	float roughness = 1.f;
	vec3 emissiveColour = vec3{0};
	float alphaMask = 1.f;
	float metalness = 1.f;
	GPUBool useNormalMap;
	GPUBool useEmissiveMap;
	GPUBool useRoughnessMap;
};

class MaterialArchetype : public RefCountedObject
{
public:
	MaterialArchetype() = default;
	MaterialArchetype(MaterialArchetypeDesc const& desc);
	RMOVE_DEFAULT(MaterialArchetype)
	RCOPY_PROTECT(MaterialArchetype)
	~MaterialArchetype();

//	virtual ~MaterialArchetype() {}
	ShaderCBData const& GetCBData(rnd::ECBFrequency freq) const
	{
		return CBData[Denum(freq)];
	}

	ShaderCBData& GetCBData(rnd::ECBFrequency freq)
	{
		return CBData[Denum(freq)];
	}
	
	virtual void Bind(rnd::RenderContext& rctx, EShadingLayer layer, EMatType matType);

	EnumArray<rnd::ECBFrequency, ShaderCBData> CBData;
	EnumArray<EShadingLayer, RefPtr<rnd::MaterialPixelShader const>> PixelShaders;
	MaterialArchetypeDesc Desc;
	String DebugName;
	mutable RefPtr<rnd::VertexShader const> mVertexShader;
	std::unique_ptr<rnd::RenderMaterial> m_Default;
	bool mGotVSCBData = false;
	bool mGotPSCBData = false;

};

using RenderMaterialType = MaterialArchetype;

class IDeviceMaterial : public RefCountedObject
{
public:
	IDeviceMaterial(RefPtr<MaterialArchetype> matType)
		: Archetype(matType){}

	virtual ~IDeviceMaterial() {}

	const RefPtr<MaterialArchetype> Archetype;
	virtual void Bind(rnd::RenderContext& rctx, EShadingLayer layer) = 0;
};


class RenderMaterial : public IDeviceMaterial
{
public:
	RenderMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity)
		: IDeviceMaterial(matType), mMatType(opacity) {}



	virtual void UnBind(rnd::RenderContext& rctx)
	{
	}

	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override;
protected:
	EMatType mMatType;
};

class TexturedRenderMaterial : public RenderMaterial
{
public:

	TexturedRenderMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity, DeviceTextureRef const& albedo = nullptr)
		: RenderMaterial(matType, opacity), m_Albedo(albedo) {}

	
	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override;
	void		 UnBind(rnd::RenderContext& rctx) override {}

	DeviceTextureRef m_Albedo;
	DeviceTextureRef m_Normal;
	DeviceTextureRef m_Emissive;
	DeviceTextureRef m_Roughness;
};


}
