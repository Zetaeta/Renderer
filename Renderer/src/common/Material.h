#pragma once

#include "core/Maths.h"
#include "asset/Texture.h"
#include <common/Material.h>
#include <atomic>
#include <mutex>

enum EMatType
{
	E_MT_OPAQUE = 0x1,
	E_MT_TRANSL = 0x2,
	E_MT_MASKED = 0x4,
	E_MT_ALL = E_MT_OPAQUE | E_MT_TRANSL | E_MT_MASKED
};

FLAG_ENUM(EMatType)

using col4 = vec4;
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
	private:
	std::atomic<bool> m_Updated = false;
	std::mutex m_UpdateMutex;
};

using MaterialID = int;

