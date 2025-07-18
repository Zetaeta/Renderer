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

struct PerInstanceVSData
{
	DECLARE_STI_NOBASE(PerInstanceVSData);
public:
	mat4 fullTransform;
	mat4 model2ShadeSpace;
	mat4 model2ShadeDual;
};
DECLARE_CLASS_TYPEINFO(PerInstanceVSData);


__declspec(align(16))
struct VS2DCBuff
{
	float2 pos;
	float2 size;
};

__declspec(align(16))
struct PS2DCBuff
{
	float exponent = 1.f;
	int renderMode = 0; // 1=depth, 2=uint
};


class MaterialArchetype : public RefCountedObject
{
public:
	MaterialArchetype() = default;
	MaterialArchetype(MaterialArchetypeDesc const& desc);
	RMOVE_DEFAULT(MaterialArchetype)
	ZE_COPY_PROTECT(MaterialArchetype)
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
	RenderMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity, StandardMatProperties const& props)
		: IDeviceMaterial(matType), mProps(props), mMatType(opacity) { }

	StandardMatProperties const& Props() const
	{
		return mProps;
	}

	virtual void UnBind(rnd::RenderContext& rctx)
	{
	}

	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override;
protected:
	EMatType mMatType;
	StandardMatProperties mProps;
};

class TexturedRenderMaterial : public RenderMaterial
{
public:

	TexturedRenderMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity, StandardMatProperties const& props, DeviceTextureRef const& albedo = nullptr)
		: RenderMaterial(matType, opacity, props), m_Albedo(albedo) {}

	
	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override;
	void		 UnBind(rnd::RenderContext& rctx) override {}

	DeviceTextureRef m_Albedo;
	DeviceTextureRef m_Normal;
	DeviceTextureRef m_Emissive;
	DeviceTextureRef m_Roughness;
};


}
