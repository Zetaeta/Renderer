#pragma once
#include "BaseObject.h"
//#include "Scene.h"
#include <assimp/scene.h>		// Output data structure
#include "Mesh.h"

using AssetPath = String;
namespace std { namespace filesystem { class directory_entry; } }

class Asset : public BaseObject
{
public:
	 using Ref = s32;
	 AssetPath file;
};

enum class EAssetType : u8
{
	MESH,
	TEXTURE
};

struct AssetRef
{
	s32 idx;
	EAssetType assType;
};

struct Scene;
struct MeshPart;


class AssetManager : public BaseObject
{
public:
	using Meshes = std::vector<Mesh>;
	
	friend struct Scene;
	Asset::Ref LoadAssetUntyped(AssetPath path);

	Asset::Ref LoadAssetUntyped(std::filesystem::directory_entry const& file);

	MeshRef LoadMesh(AssetPath path);

	MeshRef AddMesh(Mesh&& mesh);
	
	Meshes const& GetMeshes() const
	{
		return m_Meshes;
	}

	Meshes& GetMeshes()
	{
		return m_Meshes;
	}


	template <typename TAsset>
	TAsset::Ref LoadAsset(AssetPath path)
	{
		
	}

	String GetFilePath(AssetPath path);

private:
	MeshRef AddMesh(aiScene const* aiscene);
	void AddNode(std::vector<MeshPart>& components, aiNode const* node, aiMatrix4x4 transform, u32 meshOffset);
	bool LoadTexture(aiMaterial const* aimat, aiTextureType texType, TextureRef& outTexture);

	std::vector<Material> m_Materials;
	std::vector<Mesh> m_Meshes;
	std::vector<CompoundMesh> m_CompoundMeshes;
};
