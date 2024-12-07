#pragma once

#include "core/Maths.h"
#include "asset/Texture.h"
#include <common/Material.h>
#include <atomic>
#include <mutex>
#include <core/RefCounted.h>
#include <render/ConstantBuffer.h>

namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
}

enum EMatType
{
	E_MT_OPAQUE = 0x1,
	E_MT_TRANSL = 0x2,
	E_MT_MASKED = 0x4,
	E_MT_ALL = E_MT_OPAQUE | E_MT_TRANSL | E_MT_MASKED
};

FLAG_ENUM(EMatType)

enum EMatArchetype
 {
	 MAT_PLAIN = 0,
	 MAT_TEX,
	 MAT_BG,
	 MAT_2D,
	 MAT_2D_UINT,
	 MAT_POINT_SHADOW_DEPTH,
	 MAT_CUBE_DEPTH,
	 MAT_SCREEN_ID,
	 MAT_COUNT
 };

enum class EShadingLayer : u8
{
	BASE = 0,
	DIRLIGHT,
	LIGHTS_START = DIRLIGHT,
	POINTLIGHT,
	SPOTLIGHT,
	SHADING_COUNT,
	NONE = SHADING_COUNT,
	DEPTH = NONE,
	COUNT
};

struct ShaderCBData
{
	rnd::DataLayout::Ref Layout{};
	bool IsUsed = false;
};

class MaterialArchetype : public RefCountedObject
{
public:
//	virtual ~MaterialArchetype() {}
	std::array<ShaderCBData, Denum(rnd::ECBFrequency::Count)> CBData;
	ShaderCBData const& GetCBData(rnd::ECBFrequency freq) const
	{
		return CBData[Denum(freq)];
	}

	ShaderCBData& GetCBData(rnd::ECBFrequency freq)
	{
		return CBData[Denum(freq)];
	}
	
	virtual void Bind(rnd::RenderContext& rctx, EShadingLayer layer) = 0;
	String DebugName;
};

enum class EShadingLayer : u8;

class IDeviceMaterial
{
public:
	IDeviceMaterial(RefPtr<MaterialArchetype> matType)
		: Archetype(matType){}

	virtual ~IDeviceMaterial() {}

	const RefPtr<MaterialArchetype> Archetype;
	virtual void Bind(rnd::RenderContext& rctx, EShadingLayer layer) = 0;
};

struct Material
{
	RCOPY_PROTECT(Material);

	Material(Material&& other) noexcept;
	Material(String debugName, col4 col, float specularity = 1, int spec = -1, float diffuseness = 1, TextureRef albedo = {})
		: colour(col), specularExp(spec), albedo(albedo), DebugName(debugName) {}

	col4 Colour() {
		return colour;
	}
	int Specularity() {
		return specularExp;
	}

	using Ref = std::shared_ptr<Material>;

	bool NeedsUpdate() const;
	void MarkUpdated();
	void MarkUpdateDone();

	std::mutex& GetUpdateMutex() { return m_UpdateMutex; }

	void OnPropertyUpdate();

	String DebugName;
	TextureRef albedo;
	TextureRef normal;
	TextureRef emissiveMap;
	TextureRef roughnessMap;
	col4 colour;
	col3 emissiveColour = col3(0);
	int specularExp = -1;
	float specularity = 1;
	float diffuseness = 1;
	bool translucent = false;
	float mask = 1.f;
	float roughness = 0.5;
	float metalness = 0;

	EMatType GetMatType() const
	{
		if (translucent) return E_MT_TRANSL;
		if (mask < 1.f) return E_MT_MASKED;
		return E_MT_OPAQUE;
	}

	IDeviceMaterial* DeviceMat = nullptr;
private:
	std::atomic<bool> m_Updated = false;
	std::mutex m_UpdateMutex;
};

using MaterialID = int;

class MaterialManager
{
public:
	MaterialManager(rnd::IRenderDeviceCtx* deviceCtx)
		: mRenderer(deviceCtx)
	{
	}
	IDeviceMaterial* GetDefaultMaterial(int matType);

	//std::vector<OwningPtr<MaterialArchetype>> mMatTypes;
	//std::vector<OwningPtr<IDeviceMaterial>> mMaterials;
	rnd::IRenderDeviceCtx* mRenderer;
};

