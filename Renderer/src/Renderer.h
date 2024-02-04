#pragma once

#include "maths.h"

struct Scene;
class Camera;
class IRenderer
{
public:
	virtual void Render(const Scene& scene) = 0;
	virtual void Resize(u32 width, u32 height, u32* canvas) = 0;
};
