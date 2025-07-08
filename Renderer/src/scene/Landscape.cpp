#include "Landscape.h"

#include "asset/Texture.h"
#include "procedural/PerlinNoise.h"
#include <random>

Landscape::Landscape(Scene* scene, uvec2 extents /*= {1, 1}*/)
	: SceneObject(scene, "Landscape")
{

	constexpr u32 textureSize = 1024;
	mHeightmap = std::make_shared<Texture>(textureSize, textureSize, "LandscapeHeight", nullptr, ETextureFormat::R32_Float, false);
	float* data = reinterpret_cast<float*>(mHeightmap->GetMutableData());
	std::mt19937 random(0);
//	PerlinNoise2 noise(1.f, 1024, random);
	auto noise = 0.5f * PerlinNoise2(1.f, 256, random)
		+ 0.25f * PerlinNoise2(2.f, 256, random) + 0.125f * PerlinNoise2(4.f, 256, random);
		+ 0.0625f * PerlinNoise2(8.f, 256, random);
	for (int y = 0; y < textureSize; ++y)
	{
		for (int x = 0; x < textureSize; ++x)
		{
			data[y * textureSize + x] = //(
			noise.Evaluate(vec2{x, y} * 4.f/float(textureSize));// + 1)/2;
		}
	}
	mHeightmap->Upload();
}
