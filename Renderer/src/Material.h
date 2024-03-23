#pragma once

#include "maths.h"
#include "Texture.h"
#include <atomic>
#include <mutex>

using col4 = vec4;
struct Material
{
	RCOPY_PROTECT(Material);

	Material(Material&& other) noexcept;
	Material(col4 col, float specularity = 1, int spec = -1, float diffuseness = 1, TextureRef albedo = {})
		: colour(col), specularExp(spec), albedo(albedo) {}

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
private:
	std::atomic<bool> m_Updated = false;
	std::mutex m_UpdateMutex;
};

using MaterialID = int;

