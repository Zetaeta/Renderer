#include "RTRenderer.h"
#include "Scene.h"
#include <algorithm>
#include <execution>
#include <numeric>
#include <ranges>


void RTRenderer::Render(const Scene& scene)
{
	xoffset = float(m_Width)/(2.f * m_PixelScale);
	yoffset = float(m_Height)/(2.f * m_PixelScale);
	ComputeRayDirections();
	auto ys = std::ranges::views::iota(0,int(m_Height));
	std::for_each(std::execution::par, ys.begin(), ys.end(), [this, &scene](int y) {
		for (u32 x=0; x<m_Width; ++x)
		{
			m_Canvas[x + y * m_Width] = vecToRGBA(PerPixel(scene,x,y));
		}

		});
}

glm::vec4 RTRenderer::PerPixel(const Scene& scene, int x, int y)
{
	vec3 start = m_Camera->position;
	const vec3& dir = m_RayDirections[x + y * m_Width]; //= m_Camera->view * glm::normalize(vec3 { float(x) / m_PixelScale - xoffset, float(y) / m_PixelScale - yoffset, 1 });
	float t = INFINITY;
	//u32  colour{0};
	int hitSphere = -1;
	for (int i=0; i<scene.m_Spheres.size(); ++i) {
		const Sphere& sphere = scene.m_Spheres[i];
		// (start + t*dir - sphere.origin)^2 = radius^2
		float r = sphere.radius;
		float a = dot(dir , dir);
		float b = 2.f * (dot(dir , start) - dot(dir , sphere.origin));
		float c = dot(start , start) + dot(sphere.origin , sphere.origin) - 2.f * dot(start , sphere.origin) - (r * r);
		float discriminant = b*b - 4 * a * c;

		if (discriminant < 0) continue;
		float tnear = (-b - glm::sqrt(discriminant)) / (2.f * a);
		// float tfar = (-b - glm::sqrt(discriminant)) / (2.f * a);
		if (tnear > t || tnear < 0) continue;

		t = tnear;
		hitSphere = i;
	}
	int hitMesh = -1;
	vec3 hitNormal;
	for (int i = 0; i < scene.m_MeshInstances.size(); ++i)
	{
		const Mesh& mesh = scene.GetMesh(scene.m_MeshInstances[i].mesh);
		const Transform trans = scene.m_MeshInstances[i].trans;
		for (const IndexedTri& tri : mesh.triangles)
		{
			// Moller-Trumbore
			vec3 v0 = trans * mesh.vertices[tri[0]].pos, v1 = trans * mesh.vertices[tri[1]].pos, v2 = trans * mesh.vertices[tri[2]].pos;
			vec3 edge1 = v1 - v0, edge2 = v2 - v0;
			vec3 h = cross(dir, edge2);
			constexpr const float EPSILON = 0.000001f;
			// dot(start + t*dir -v0 , normal = 0)
			float a = dot(edge1, h);
			constexpr const bool BACK_FACE_CULL = true;
			if constexpr (BACK_FACE_CULL) {
				if (a < 0) {
					continue;
				}
			}
			float overA = 1/a;
			if (a < EPSILON && a > -EPSILON) continue;
			vec3 s = start - v0;
			float u = dot(s,h) * overA;
			if (u < 0.f || u > 1.f) {
				continue;
			}
			vec3 q = cross(s, edge1);
			float v = dot(dir, q) * overA;
			if (v<0.f || u+v > 1.f) continue;
			float t1 = dot(edge2, q) * overA;
			if (t1 < t && t1 > EPSILON) {
				vec3 normal = cross(edge1,edge2);
				t = t1;
				hitMesh = i;
				hitNormal = normal;
			}
		}
	}
	vec4 colour;
	int spec;
	if (hitMesh != -1) {
		const Mesh& mesh = scene.GetMesh(scene.m_MeshInstances[hitMesh].mesh);
		colour = GetColour(mesh.material);
		spec = int(GetSpecularity(mesh.material));
		hitNormal = normalize(hitNormal);
	}
	else if (hitSphere != -1) {
		const Sphere& sphere = scene.m_Spheres[hitSphere];
		hitNormal = glm::normalize(start + t * dir - sphere.origin);
		spec = int(GetSpecularity(sphere.material));
		colour = GetColour(sphere.material);
	}
	else
	{
		return vec4(0);
	}
//	return vec4(max(hitNormal,vec3(0)), 1);
	colour *= vec4(ComputeLighting(scene, start + t * dir, hitNormal, dir, spec),1);
	colour = glm:://clamp(colour, vec4(0), vec4{ 1 });
	min(colour, vec4{1});
	
	return colour; //scene.m_Spheres[hitSphere].colour;
}

col3 RTRenderer::ComputeLighting(const Scene& scene, const vec3& point, const vec3& normal, const vec3& rayDir, int specularity)
{
	col3 intensity = scene.m_AmbientLight;
	for (auto dirLight : scene.m_DirLights) {
		intensity +=  max(dot(-dirLight.dir, normal),0.f) * dirLight.colour;
		if (specularity >= 0) {
			float l = glm::length(dirLight.dir);
			//assert(l < 1.1f);
			vec3 outRay = dirLight.dir - 2 * dot(dirLight.dir, normal) * normal;
			//assert(dot(outRay, -rayDir) < 1.0001f);
			col3 specLight = float(pow(max(dot(outRay, -rayDir),0.f), specularity)) * dirLight.colour;
			//return specLight;
			intensity += specLight;
		}
	}
	return intensity;
}

void RTRenderer::ComputeRayDirections()
{
	if (!m_Camera->ViewChanged() && !m_Dirty) {
		return;
	}
	m_Camera->CleanView();
	m_RayDirections.resize(m_Width * m_Height);
	for (u32 y=0; y<m_Height; ++y)
	{
		for (u32 x=0; x<m_Width; ++x)
		{
			m_RayDirections[x + y * m_Width] = m_Camera->view * glm::normalize(vec3 { float(x) / m_PixelScale - xoffset, float(y) / m_PixelScale - yoffset, 1 });;
		}
	}
	m_Dirty = false;
}

void RTRenderer::Resize(u32 width, u32 height, u32* canvas)
{
	m_Width = width;
	m_Height = height;
	m_Canvas = canvas;
	m_Dirty = true;
}
