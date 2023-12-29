#pragma once

#include "maths.h"
#include "Texture.h"

using Colour_t = vec4;
struct Material
{
	Material(Colour_t col, int spec = -1, TextureRef albedo = {})
		: colour(col), specularity(spec), albedo(albedo) {}

	Colour_t Colour() {
		return colour;
	}
	int Specularity() {
		return specularity;
	}

	TextureRef albedo;
	Colour_t colour;
	int specularity = -1;
};

using MaterialID = int;


