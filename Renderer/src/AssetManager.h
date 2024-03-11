#pragma once
#include "BaseObject.h"
//#include "Scene.h"
#include <assimp/scene.h>		// Output data structure
#include "Mesh.h"
#include "Asset.h"
#include <filesystem>
#include <queue>
#include <condition_variable>

namespace std { namespace filesystem { class directory_entry; } }

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

inline std::string ToString(std::filesystem::path const& path)
{
	std::string result = path.string();
	std::replace(result.begin(), result.end(), '\\', '/');
	return result;
}

template<typename T>
class MessageQueue
{
public:
	template<typename TArg>
	void push(TArg&& t)
	{
		{
			std::lock_guard lock {m_Mutex};
			m_Queue.push(t);
		}
		m_Cv.notify_one();
	}

	T pop()
	{
		std::unique_lock lock {m_Mutex};
		m_Cv.wait(lock, [&] { return !m_Queue.empty(); });
		T t = std::move(m_Queue.front());
		m_Queue.pop();
		return t;
	}
		
private:
	std::mutex m_Mutex;
	std::condition_variable m_Cv;
	std::queue<T> m_Queue;
};

using AssetLoadCallback = std::function<void(AssetRef)>;

class AssetManager : public BaseObject
{
public:
	AssetManager() { RefreshMeshList(); }
	using Meshes = std::vector<Mesh>;
	
	friend struct Scene;
	Asset::Ref LoadAssetUntyped(AssetPath path);

	Asset::Ref LoadAssetUntyped(std::filesystem::directory_entry const& file);

	MeshRef ImportMesh(String const& path, bool loadTextures = true);
	void ImportMeshAsync(String const& path, bool loadTextures = true, AssetLoadCallback callback = nullptr);

	MeshRef LoadMesh(AssetPath const& path, bool loadTextures = true);
	void LoadMeshAsync(AssetPath const& path, bool loadTextures = true, AssetLoadCallback callback = nullptr);

	MeshRef AddMesh(Mesh&& mesh);
	
	Meshes const& GetMeshes() const
	{
		return m_Meshes;
	}

	Meshes& GetMeshes()
	{
		return m_Meshes;
	}

	Mesh& GetMesh(MeshRef ref)
	{
		return m_Meshes[ref];
	}

	MeshRef GetMesh(AssetPath path);

	AssetPath GetMeshPath(MeshRef); 

	MeshRef FindMesh(AssetPath path);

	template <typename TAsset>
	TAsset::Ref LoadAsset(AssetPath path)
	{
		
	}

	void RefreshMeshList();

	struct AssetData
	{
		AssetPath path;
		bool loaded = false;
	};

	Vector<AssetData> const& MeshList() { return m_AllMeshes; }

	String GetFilePath(std::filesystem::path const& path);

	void DrawControls();


private:
	
	struct LoadingJob
	{
		EAssetType m_Type;
		String path;
		bool isImport;
		bool loadTextures = true;
		AssetLoadCallback cb;
	};

	MessageQueue<LoadingJob> m_LoadingJobs;

	MeshRef AddMesh(aiScene const* aiscene, AssetPath const& path, bool isImport, bool loadTextures = true);
	void AddNode(std::vector<MeshPart>& components, aiNode const* node, aiMatrix4x4 transform, u32 meshOffset);
	bool LoadMatTexture(aiMaterial* aimat, aiTextureType texType, TextureRef& outTexture, AssetPath const& meshPath, bool isImport);

	std::vector<Material> m_Materials;
	std::vector<Mesh> m_Meshes;
	std::vector<CompoundMesh> m_CompoundMeshes;

	std::vector<AssetData> m_AllMeshes;

	std::unordered_map<AssetPath, TextureRef> m_LoadedTextures;

	bool m_PreTransform = true;
};
