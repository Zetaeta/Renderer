#pragma once

#include "core/Maths.h"

struct Scene;
class UserCamera;
class IRenderer
{
public:
	virtual void DrawControls() {};
	virtual void Render(const Scene& scene) = 0;
	virtual void Resize(u32 width, u32 height, u32* canvas) = 0;
};
