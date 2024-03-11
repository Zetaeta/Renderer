#pragma once
#include <vector>
#include <string>
#include <array>
#include "maths.h"
#include "Material.h"
#include "Transform.h"
#include "Types.h"
#include "Asset.h"

struct TVertex
{
	TVertex(vec3 pos = vec3 {0}, vec3 normal = vec3{ 0 }, vec2 uvs = vec2{ 0 })
		:pos(pos), normal(normal), uvs(uvs){}
	pos3 pos;
	vec3 normal;
	vec3 tangent;
	vec2 uvs;
};
using Vertex = TVertex;

using IndexedTri = std::array<u16, 3>;

struct Mesh : public Asset
{
	template <typename TVert = std::vector<Vertex>, typename TInd = std::vector<IndexedTri>>
	Mesh(AssetPath const& path, TVert&& verts, TInd&& tris, std::string name = "", MaterialID mat = 0)
		: Asset(path)
		, vertices(std::forward<TVert>(verts))
		, triangles(std::forward<TInd>(tris))
		, material(mat)
		, name(name) {}

	static Mesh MakeCube(vec3 origin, mat3 rotation, MaterialID mat);

	std::vector<Vertex>		vertices;
	std::vector<IndexedTri> triangles;
	MaterialID				material;

	std::string name;

	static Mesh Cube(MaterialID m = 0);
};

using MeshRef = s32;
constexpr MeshRef INVALID_MESH_REF = -1;


struct MeshInstance
{
	MeshRef mesh;
	Transform trans;
};

using MeshInstanceRef = s32;

struct MeshPart
{
	MeshInstanceRef instance;
	String name;
	RotTransform trans;

};

struct CompoundMesh
{
	String name;
	std::vector<MeshPart> components;
};

struct SceneletPart
{
	CompoundMesh mesh;
	RotTransform trans;
};

struct Scenelet
{
	Vector<SceneletPart> parts;
};

