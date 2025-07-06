#pragma once
#include "SceneObject.h"

class Texture;

class Landscape : public SceneObject
{
public:
	Landscape(Scene* scene, uvec2 extents = uvec2{1, 1});
private:
	std::shared_ptr<Texture> mHeightmap;
};
