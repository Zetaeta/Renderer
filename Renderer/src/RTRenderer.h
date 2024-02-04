#pragma once
#include "Renderer.h"
#include "glm/fwd.hpp"
#include "Camera.h"
#include <vector>
#include "Scene.h"

using namespace glm;

/* struct Ray
{
	vec3 origin
};*/

class RTRenderer : public IRenderer
{
public:
	RTRenderer(Camera* camera, u32 width, u32 height, u32* canvas)
		: m_Canvas(canvas), m_Height(height), m_Width(width), m_Camera(camera), m_PixelWidth(width)
	{}
	void Render(const Scene& scene) override;
	vec4 PerPixel(const Scene& scene, int x, int y);

	float ComputeLighting(const Scene& scene, const vec3& point, const vec3& normal , const vec3& rayDir, int specularity);

	void ComputeRayDirections();


	void Resize(u32 width, u32 height, u32* canvas) override;

/* inline static Colour_t GetColour(MaterialID mat)
	{
		return mat.colour;
	}
	inline static int GetSpecularity(MaterialID mat) {
		return mat.specularity;
	}*/
#define GetColour(mat) \
	scene.m_Materials[mat].colour
#define GetSpecularity(mat) \
	scene.m_Materials[mat].specularity

private:

	std::vector<vec3> m_RayDirections;

	Camera* m_Camera;

	u32* m_Canvas;
	u32 m_Width;
	u32 m_Height;
	std::vector<u32> m_PixelWidth;
	bool m_Dirty = false;

	u32 m_PixelScale = 500;
	float xoffset;
	float yoffset;
};
