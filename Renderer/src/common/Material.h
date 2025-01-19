#pragma once

#include "core/Maths.h"
#include "asset/Texture.h"
#include <common/Material.h>
#include <atomic>
#include <mutex>
#include <core/RefCounted.h>
#include <render/ConstantBuffer.h>
#include "container/EnumArray.h"
#include "render/shaders/MaterialShader.h"

namespace rnd { class RenderMaterial; }

namespace rnd { class VertexShader; }

namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
class IRenderDevice;
class ShaderManager;
class IShaderReflector;
enum VertexAttributeMask : u64;
class IDeviceMaterial;
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
	GBuffer,
	ForwardRenderCount = GBuffer,
	SHADING_COUNT,
	NONE = SHADING_COUNT,
	Depth = NONE,
	Count
};


struct ShaderCBData
{
	rnd::DataLayout::Ref Layout{};
	bool IsUsed = false;
};

struct MaterialArchetypeDesc
{
//	rnd::ShaderDesc mShaderDesc;
	String DebugName;
	rnd::ShaderTypeId mVSRegistryId = 0;
	rnd::ShaderTypeId mPSRegistryId = 0;
	rnd::VertexAttributeMask mVertexMask {};
	rnd::MaterialPixelShader const* GetShader(rnd::ShaderManager& shaderMgr, EShadingLayer layer, EMatType opacity, OwningPtr<rnd::IShaderReflector>* outReflector = nullptr) const;
	rnd::MaterialVertexShader const* GetVertexShader(rnd::ShaderManager& shaderMgr, OwningPtr<rnd::IShaderReflector>* outReflector = nullptr) const;
};

MaterialArchetypeDesc CreateAndRegisterMatDesc(const char* name, const char* psFilename, const char* psEntryPoint,
	const char* vsFilename, const char* vsEntryPoint, u64 vertexMask);

#define DECLARE_MATERIAL_SHADER(name) extern MaterialArchetypeDesc name;
#define DEFINE_MATERIAL_SHADER(Name, ...)\
	MaterialArchetypeDesc Name = CreateAndRegisterMatDesc(#Name, __VA_ARGS__);

using MaterialID = int;

struct StandardMatProperties
{
	col4 colour {};
	col3 emissiveColour = col3(0);
	int specularExp = -1;
	float specularity = 1;
	float diffuseness = 1;
	float mask = 1.f;
	float roughness = 0.5;
	float metalness = 0;
	bool translucent = false;

	EMatType GetMatType() const
	{
		if (translucent) return E_MT_TRANSL;
		if (mask < 1.f) return E_MT_MASKED;
		return E_MT_OPAQUE;
	}

};

struct Material
{
	RCOPY_PROTECT(Material);

	Material(Material&& other) noexcept;
	Material(String debugName, col4 col, float specularity = 1, int spec = -1, float diffuseness = 1, TextureRef albedo = {})
		: albedo(albedo), DebugName(debugName), IsTextured(albedo.IsValid())
	{
		Props.colour = col;
		Props.specularExp = spec;
	}

	col4 Colour() {
		return Props.colour;
	}
	int Specularity() {
		return Props.specularExp;
	}

	using Ref = std::shared_ptr<Material>;

	constexpr static MaterialID DefaultMaterial = 0;

	bool NeedsUpdate() const;
	void MarkUpdated();
	void MarkUpdateDone();

	std::mutex& GetUpdateMutex() { return m_UpdateMutex; }

	void OnPropertyUpdate();

	EMatType GetMatType() const
	{
		return Props.GetMatType();
	}

	String DebugName;
	TextureRef albedo;
	TextureRef normal;
	TextureRef emissiveMap;
	TextureRef roughnessMap;
	StandardMatProperties Props;
	bool IsTextured = false;
	MaterialID Id;

	rnd::IDeviceMaterial* DeviceMat = nullptr;
private:
	std::atomic<bool> m_Updated = false;
	std::mutex m_UpdateMutex;
};

