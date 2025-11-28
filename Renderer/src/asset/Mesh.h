#pragma once
#include <vector>
#include <string>
#include <array>
#include "core/Maths.h"
#include "common/Material.h"
#include "core/Transform.h"
#include "core/Types.h"
#include "Asset.h"
#include "render/VertexAttributes.h"

struct Vertex
{
	Vertex(vec3 pos = vec3 {0}, vec3 normal = vec3{ 0 }, vec2 uvs = vec2{ 0 })
		:pos(pos), normal(normal), uvs(uvs){}
	pos3 pos;
	vec3 normal;
	vec3 tangent;
	vec2 uvs;
};
DECLARE_VERTEX_ATTRIBUTE_DESC(Vertex);

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
	template <typename TVert = std::vector<Vertex>, typename TInd = std::vector<IndexedTri>>
	MeshPart(TVert&& verts, TInd&& tris, MaterialID mat = 0, AssetPath name = {})
		: vertices(std::forward<TVert>(verts))
		, triangles(std::forward<TInd>(tris))
		, material(mat)
		, name(name) {}

	u32 GetVertCount() const
	{
		return NumCast<u32>(vertices.size());
	}

	u32 GetTriCount() const
	{
		return NumCast<u32>(triangles.size());
	}
	std::vector<Vertex>		vertices;
	std::vector<IndexedTri> triangles;
	MaterialID				material;

	AssetPath name;
	static MeshPart Cube(MaterialID m = 0);
};


/// <summary>
/// A compound mesh is collection of individual meshes that share an origin.
/// </summary>
struct CompoundMesh : public Asset
{
	ZE_COPY_PROTECT(CompoundMesh);
	RMOVE_DEFAULT(CompoundMesh);
	CompoundMesh(AssetPath const& path);
	CompoundMesh(AssetPath const& path, MeshPart&& mesh);
	HashString name;
	std::vector<MeshPart> components;

	using Ref = std::shared_ptr<CompoundMesh>;
	inline static Ref INVALID_REF = nullptr;
};

struct SceneletPart
{
	CompoundMesh::Ref mesh = CompoundMesh::INVALID_REF;
	std::vector<SceneletPart> children;
	RotTransform trans;

	CompoundMesh::Ref FindMesh(String const& name);
};

/// <summary>
/// A scenelet represents an asset file, which may contain many meshes transformed.
/// Meshes that share a transform are grouped into a single CompoundMesh, which theoretically
/// represents a single object with subparts for different materials.
/// </summary>
struct Scenelet : public Asset
{
	Scenelet(AssetPath const& path)
		: Asset(path), m_Name(path.SubPath) {}
	CompoundMesh::Ref FindMesh(String const& name);
	SceneletPart m_Root;
	HashString m_Name;
	using Ref = s32;
};

