#pragma once
#include <core/BaseObject.h>
//#include "scene/Scene.h"
#include <assimp/scene.h>		// Output data structure
#include "asset/Mesh.h"
#include "Asset.h"
#include <filesystem>
#include <queue>
#include <condition_variable>
#include <variant>


namespace std {
class thread;
namespace filesystem { class directory_entry; } }

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

class Scene;
struct MeshPart;

inline std::string ToString(std::filesystem::path const& path)
{
	std::string result = path.string();
	std::replace(result.begin(), result.end(), '\\', '/');
	return result;
}

template<typename T>
struct Locked
{
	template<typename TFunc>
	void Access(TFunc&& func)
	{
		std::lock_guard l {m_Mutex};
		func(m_Object);
	}

	std::pair<T&,std::unique_lock<std::mutex>> Acquire() { return {m_Object, std::unique_lock(m_Mutex)}; }

private:
	T m_Object;
	std::mutex m_Mutex;
};

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

	bool IsEmpty()
	{
		std::lock_guard lock {m_Mutex};
		return m_Queue.empty();
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
	AssetManager();
	~AssetManager();
	using Meshes = std::vector<Mesh>;
	
	friend Scene;
	Asset::Ref LoadAssetUntyped(AssetPath path);

	Asset::Ref LoadAssetUntyped(std::filesystem::directory_entry const& file);

	Scenelet::Ref ImportScenelet(String const& path, bool loadTextures = true, bool async = false);
	void ImportSceneletAsync(String const& path, bool loadTextures = true, AssetLoadCallback callback = nullptr);

	Scenelet::Ref LoadScenelet(AssetPath const& path, bool loadTextures = true, bool async = false);
	void LoadSceneletAsync(AssetPath const& path, bool loadTextures = true, AssetLoadCallback callback = nullptr);

	CompoundMesh::Ref AddMesh(CompoundMesh&& mesh);
	
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

	Vector<Scenelet>& GetScenelets() { return m_Scenelets; }

	CompoundMesh::Ref GetMesh(AssetPath const& path);
	CompoundMesh&	  GetMesh(CompoundMesh::Ref mesh) { return *mesh; }

	AssetPath GetMeshPath(MeshRef); 

	MeshRef FindMesh(AssetPath const& path);

	template <typename TAsset>
	TAsset::Ref LoadAsset(AssetPath path)
	{
		
	}

	Material& GetMaterial(MaterialID const& mid) { return *m_Materials[mid]; }
	Material& GetMaterial(Material::Ref const& ref) { return *ref; }

	void RefreshMeshList();

	struct AssetData
	{
		AssetPath path;
		bool loaded = false;
	};

	Vector<AssetData> const& MeshList() { return m_AllMeshes; }

	String GetFilePath(std::filesystem::path const& path);

	void DrawControls();

	// Async
	void Start();
	void Finish();
	void Synchronize();
	void Work();

	void Tick();

private:
	
	struct SceneletLoadingJob
	{
		String path;
		bool isImport;
		bool loadTextures = true;
		AssetLoadCallback cb = nullptr;
	};

	struct FinishJob
	{
	};

	struct MatLoadingJob
	{
		Material::Ref m_Mat = nullptr;
		String m_Albedo;
		String m_Normal;
		String m_Emissive;
		String m_Roughness;
		bool m_Import = false;
		String m_MeshPath;
		AssetLoadCallback cb = nullptr;
	};

	using LoadingJob = std::variant<SceneletLoadingJob, MatLoadingJob, FinishJob>;

	MessageQueue<LoadingJob> m_LoadingJobs;

	struct SceneletLoadCtx
	{
		Scenelet m_Scenelet;
		Vector<CompoundMesh::Ref> m_Meshes; 
	};

	void Perform(MatLoadingJob& job);

	void FinishLoad(SceneletLoadCtx& ctx);

	MessageQueue<SceneletLoadCtx> m_LoadedScenelets;
		
	Scenelet::Ref AddScenelet(aiScene const* aiscene, AssetPath const& path, bool isImport, String const& importPath, bool loadTextures, bool async);
	void AddNode(SceneletLoadCtx& ctx, SceneletPart& parent, Vector<MeshPart> const& meshParts, aiNode const* node, aiMatrix4x4 transform);
	bool LoadMatTexture(String const& path, TextureRef& outTexture, String const& meshPath, bool isImport);

	s32 FindScenelet(AssetPath const& path);

	std::vector<Material::Ref> m_Materials;
	std::vector<Mesh> m_Meshes;
	std::vector<CompoundMesh::Ref> m_CompoundMeshes;

	std::vector<Scenelet> m_Scenelets;

	std::vector<AssetData> m_AllMeshes;

	Locked<std::unordered_map<AssetPath, TextureRef>> m_LoadedTextures;

//	std::unordered_map<AssetPath, Asset*> mLoadedAssets;
	
	std::unordered_map<AssetPath, CompoundMesh::Ref> m_LoadedMeshes;

	bool m_PreTransform = true;
	OwningPtr<std::thread> m_WorkerThread;
};
