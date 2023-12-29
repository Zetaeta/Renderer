#pragma once
#include <vector>
#include <array>
#include "maths.h"
#include "Transform.h"
#include "Texture.h"
#include "Material.h"

using namespace glm;


struct Sphere
{

	Sphere(vec3 origin, float rad, MaterialID mat)
		: origin(origin), radius(rad), material(mat) {}

	vec3 origin;
	float radius;
	MaterialID material;
	//vec4 colour;
};

/*struct IndexedTri
{

};*/

struct Vertex
{
	Vertex(vec3 pos = vec3 {0}, vec3 normal = vec3{ 0 }, vec2 uvs = vec2{ 0 })
		:pos(pos), normal(normal), uvs(uvs){}
	vec3 pos;
	vec3 normal;
	vec2 uvs;
};

using IndexedTri = std::array<u32, 3>;

struct Mesh
{
	template <typename TVert = std::vector<Vertex>, typename TInd = std::vector<IndexedTri>>
	Mesh(TVert&& verts, TInd&& tris, std::string name = "", MaterialID mat = 0)
		: vertices(std::forward<TVert>(verts))
		, triangles(std::forward<TInd>(tris))
		, material(mat)
		, name(name) {}

	static Mesh MakeCube(vec3 origin, mat3 rotation, MaterialID mat);

	std::vector<Vertex> vertices;
	std::vector<IndexedTri> triangles;
	MaterialID material;

	std::string name;

	static Mesh Cube(MaterialID m = 0);
};

struct DirLight
{
	DirLight(const vec3& dir,float intensity = 1.f)
		: dir(glm::normalize(dir)), intensity(intensity) {}

	 operator vec3() {
		 return dir;
	 }

	vec3 dir;
	float intensity;
};


struct MeshInstance
{
	u32 mesh;
	Transform trans;
};

struct Scene
{
	std::vector<Material> m_Materials;
	std::vector<Mesh> m_Meshes;
	std::vector<MeshInstance> m_MeshInstances;

	std::vector<Sphere> m_Spheres;
	std::vector<DirLight> m_DirLights;
	float				  m_AmbientLight = 0.2;
};
