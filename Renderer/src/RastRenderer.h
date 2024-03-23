#pragma once
#include "Renderer.h"
#include <vector>
#include "Scene.h"
#include <array>

struct ClipSpaceVertex
{
	vec4 pos;
	vec3 normal;
	vec2 uvs;
	vec3 worldPos;

	ClipSpaceVertex operator+(ClipSpaceVertex const& other) const
	{
		return {pos + other.pos, glm::normalize(normal + other.normal), uvs + other.uvs, worldPos + other.worldPos};
	}

	ClipSpaceVertex operator*(float f) const
	{
		return {f * pos, f * normal, f * uvs, f * worldPos};
	}
};

inline ClipSpaceVertex operator*(float f, ClipSpaceVertex const& v)
{
	return v * f;
}

class RastRenderer : public IRenderer
{
public:
	RastRenderer(Camera* camera, u32 width, u32 height, u32* canvas)
		: m_Canvas(canvas), m_Height(height), m_Width(width), m_Camera(camera), m_PixelWidth(width), m_DepthBuffer(width * height)
	{}
	void Render(const Scene& scene) override;

	void DrawLine(vec2 a, vec2 b);
	void DrawTri(mat4 const& fullTrans, mat4 const& model2World, std::array<ClipSpaceVertex, 3> const& verts, MaterialID mat);

	col3 ComputeLighting(const vec3& normal , const vec3& rayDir, Material const& mat);

//	template<u32 NumPlanes, typename TIt>
//	void RecursiveClip(TIt const& planeBegin, TIt const& planeEnd, ClipSpaceTri const& verts);

	// returns number of output tris;


	inline vec2 Project(vec3 v) {
		return {v.x / v.z * m_Scale, v.y / v.z * m_Scale};
	} 
	inline vec2 Project(vec4 v) {
		//return {v.x / v.z * m_Scale, v.y / v.z * m_Scale};
		vec4  projected = m_Projection * v;
		printf("z: %f, w: %f\n", projected.z, projected.w);
		return vec2(projected) / projected.z;
	} 

	void Resize(u32 width, u32 height, u32* canvas) override;

/* inline static Colour_t GetColour(MaterialID mat)
	{
		return mat.colour;
	}
	inline static int GetSpecularity(MaterialID mat) {
		return mat.specularity;
	}*/
#define GetColour(mat) \
	scene.GetMaterial(mat).colour
#define GetSpecularity(mat) \
	scene.GetMaterial(mat).specularity

private:

	std::vector<float> m_DepthBuffer;

	Camera* m_Camera;

	u32* m_Canvas;

	float m_Scale;
	u32 m_Width;
	u32 m_Height;
	std::vector<u32> m_PixelWidth;
	bool m_Dirty = false;

	u32 m_PixelScale = 500;
	float xoffset;
	float yoffset;
	mat4 m_Projection;

	Scene const* m_Scene;
};

#define IND(x, y) (x + y * m_Width)
inline vec3 Vec3(vec2 v) {
	return vec3(v.x, v.y, 0);
}

using Plane = vec4;
using ClipSpaceTri = std::array<ClipSpaceVertex, 3>;
u32 ClipTri(ClipSpaceTri const& tri, Plane const& plane, std::array<ClipSpaceTri, 2>& outClippedTris);


template<u32 NumPlanes, typename TIt, typename TFunc>
inline typename std::enable_if_t<NumPlanes != 0, void> RecursiveClip(TIt const& planeBegin, TIt const& planeEnd, ClipSpaceTri const& verts, TFunc const& func)
{
	Plane const& plane = *planeBegin;
	std::array<ClipSpaceTri, 2> clippedTris;
	u32 triCount = ClipTri(verts, plane, clippedTris);
	for (u32 i = 0; i < triCount; ++i)
	{
		RecursiveClip<NumPlanes - 1, TIt>(planeBegin + 1, planeEnd, clippedTris[i], func);
	}
}

template<u32 NumPlanes, typename TIt, typename TFunc>
inline typename std::enable_if_t<NumPlanes == 0, void> RecursiveClip(TIt const&, TIt const&, ClipSpaceTri const& tri, TFunc const& func) {
	func(tri);
}
