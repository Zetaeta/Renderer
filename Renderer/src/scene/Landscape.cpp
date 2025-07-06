#include "Landscape.h"

#include "asset/Texture.h"
#include "procedural/PerlinNoise.h"
#include <random>

Landscape::Landscape(Scene* scene, uvec2 extents /*= {1, 1}*/)
	: SceneObject(scene, "Landscape")
{
	mHeightmap = std::make_shared<Texture>(256, 256, "LandscapeHeight", nullptr, ETextureFormat::R32_Float, false);
	float* data = reinterpret_cast<float*>(mHeightmap->GetMutableData());
	PerlinNoise2 noise(1.f, 256, std::mt19937(0));
	for (int y = 0; y < 256; ++y)
	{
		for (int x = 0; x < 256; ++x)
		{
			data[y * 256 + x] = noise.Evaluate({x + 0.5, y + 0.5});
		}
	}
	mHeightmap->Upload();
}
