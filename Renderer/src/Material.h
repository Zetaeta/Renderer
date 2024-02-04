#pragma once

#include "maths.h"
#include "Texture.h"

using Colour_t = vec4;
struct Material
{
	Material(Colour_t col, float specularity = 1, int spec = -1, float diffuseness = 1, TextureRef albedo = {})
		: colour(col), specularExp(spec), albedo(albedo) {}

	Colour_t Colour() {
		return colour;
	}
	int Specularity() {
		return specularExp;
	}

	TextureRef albedo;
	TextureRef normal;
	TextureRef emissive;
	Colour_t colour;
	int specularExp = -1;
	float specularity = 0;
	float diffuseness = 1;
};

using MaterialID = int;


class MaterialManager
{
	 
};
