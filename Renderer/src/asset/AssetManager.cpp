#include "asset/AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/GltfMaterial.h>
#include <format>
#include <iostream>
#include <filesystem>
#include "core/Utils.h"
#include "imgui.h"
#include <nfd.h>
#include <ranges>
#include <thread>

using std::string;
using std::vector;
using std::cout;
using std::endl;

namespace fs = std::filesystem;

inline vec3 Vec3(aiVector3D const& v)
{
	return {v.x, v.y, v.z};
}

inline vec2 Vec2(aiVector3D const& v)
{
	return {v.x, v.y};
}

RotTransform Mat2Trans(aiMatrix4x4 const& mat)
{
	aiVector3D scale, pos, rot;
	mat.Decompose(scale, rot, pos);
	vec3 _scale = Convert<vec3>(scale);
	vec3 _pos = Convert<vec3>(pos);
	Rotator _rot;
	constexpr float R2D =  float(180. / M_PI);
	_rot.yaw = rot.y * R2D;
	_rot.pitch = rot.x * R2D;
	_rot.roll = rot.z * R2D;
	return RotTransform(_pos, _scale, _rot);
}

bool AssetManager::LoadMatTexture(String const& texPath, TextureRef& outTexture, String const& meshPath, bool isImport)
{
	if (texPath.empty())
	{
		return false;
	}

	printf("Loading texture %s\n", texPath.c_str());
	if (isImport)
	{
		fs::path mp = meshPath;
		fs::path meshDir = mp.parent_path();
		if (mp.is_relative())
		{
			meshDir /= mp.parent_path();
		}
		printf("Looking in directory %s\n", ToString( meshDir).c_str());
			//			fs::path meshDir = meshPath;
		fs::path textureDirs[] = { "assets", "assets/textures", meshDir, meshDir / "textures" };
		for (fs::path const& dir : textureDirs)
		{
			auto path = dir / texPath;
			if (!fs::exists(path))
			{
				continue;
			}
			outTexture = Texture::LoadFrom(ToString(path).c_str());
			if (!outTexture.IsValid())
			{
				printf("Failed to import texture %s\n", ToString(path).c_str());
				continue;
			}
			fs::path assetPath = "textures";
			assetPath /= path.filename();
			fs::path newFilePath = "assets/textures";
			//if (!meshPath.empty())
			//{
			//	assetPath /= meshPath;
			//}
			fs::create_directories(newFilePath);
			newFilePath /= path.filename();
			if (!fs::exists(newFilePath) || fs::last_write_time(newFilePath) < fs::last_write_time(path))
			{
				printf("Copying texture\n");
				fs::copy(path, newFilePath, fs::copy_options::overwrite_existing);
			}
			printf("Loaded texture %s\n", ToString(path).c_str());
			m_LoadedTextures.Access([&] (auto& map) {
				map[AssetPath::ContentFile(assetPath.string())] = outTexture;
			});
			
			return true;
		}
		fprintf(stderr, "Failed to find texture!\n");
	}
	else
	{
		{
			auto maplock = m_LoadedTextures.Acquire();
			auto res = maplock.first.find(isImport ? AssetPath::File(texPath) : AssetPath::ContentFile(texPath));
			if (res != maplock.first.end())
			{
				outTexture = res->second;
				printf("Already loaded texture %s", texPath.c_str());
				return true;
			}
		}
		fs::path assetPath = "assets/textures";
		assetPath /= texPath;
		outTexture = Texture::LoadFrom(ToString(assetPath).c_str());
		if (!outTexture.IsValid())
		{
			printf("Failed to load texture %s\n", texPath.c_str());
			return false;
		}
		return true;
	}
	return false;
		//string file = std::format("content/{} ", diffusePath.C_Str());
		//outTexture = Texture::LoadFrom(file.c_str());
		//return outTexture->IsValid();

}

void AssetManager::Perform(MatLoadingJob& job)
{
	Material& mat = GetMaterial(job.m_Mat);
	TextureRef albedo, normal, emissive, roughness;
	LoadMatTexture(job.m_Albedo,	albedo, job.m_MeshPath, job.m_Import);
	LoadMatTexture(job.m_Normal,	normal, job.m_MeshPath, job.m_Import);
	LoadMatTexture(job.m_Emissive,	emissive, job.m_MeshPath, job.m_Import);
	LoadMatTexture(job.m_Roughness, roughness, job.m_MeshPath, job.m_Import);
	{
		std::lock_guard lock{ mat.GetUpdateMutex() };
		mat.albedo = albedo;
		mat.normal = normal;
		mat.emissiveMap = emissive;
		mat.roughnessMap = roughness;
	}
	mat.MarkUpdated();
}


s32 AssetManager::FindScenelet(AssetPath const& path)
{
	for (int i=0; i<m_Scenelets.size(); ++i)
	{
		if (path == m_Scenelets[i].m_Path)
		{
			return i;
		}
	}
	return -1;
}

/************************************************************************/
/*                       MESH LOADING                                   */
/************************************************************************/
void AssetManager::AddNode(SceneletLoadCtx& ctx, SceneletPart& parent, Vector<MeshPart> const& meshParts, aiNode const* node, aiMatrix4x4 transform)
{
	printf("Checking node %s with %d meshes, %d children\n", node->mName.C_Str(), node->mNumMeshes, node->mNumChildren);
	auto totalTrans =  node->mTransformation * transform;
	//auto trans = Mat2Trans( totalTrans);
	SceneletPart& part = parent.children.emplace_back();
	part.mesh = ctx.m_Meshes.emplace_back(std::make_shared<CompoundMesh>(ctx.m_Scenelet.GetPath().WithSubpath(node->mName.C_Str())));
	for (u32 i=0; i<node->mNumMeshes; ++i)
	{
		u32 mesh = node->mMeshes[i];
		part.mesh->components.emplace_back(meshParts[mesh]);
	}

	part.mesh->name = node->mName.C_Str();
	part.trans = Mat2Trans(node->mTransformation);
	for (u32 i=0; i<node->mNumChildren; ++i)
	{
		AddNode(ctx, part, meshParts, node->mChildren[i], totalTrans);
	}
}

void AssetManager::FinishLoad(SceneletLoadCtx& ctx)
{
	m_CompoundMeshes.insert(m_CompoundMeshes.end(), std::make_move_iterator(ctx.m_Meshes.begin()), std::make_move_iterator(ctx.m_Meshes.end()));
	m_Scenelets.emplace_back(std::move(ctx.m_Scenelet));
}

String GetAndUpdateTexture(aiMaterial* mat, aiTextureType type, bool isImport)
{
	aiString texPath;
	if (mat->GetTexture(type, 0,&texPath) != aiReturn_SUCCESS)
	{
		return "";
	}
	if (isImport)
	{
		fs::path path = texPath.C_Str();
//		path = "assets/textures" / path;
		aiString newPath(path.filename().string().c_str());

		mat->AddProperty(&newPath, AI_MATKEY_TEXTURE(type, 0));
	}
	return texPath.C_Str();
}

Scenelet::Ref AssetManager::AddScenelet(aiScene const* aiscene, AssetPath const& path, bool isImport, String const& importPath, bool loadTextures /*= true*/, bool async )
{
	auto* root = aiscene->mRootNode;
//	auto& cmesh = m_CompoundMeshes.emplace_back(string(root->mName.C_Str()));

	u32 matStart = NumCast<u32>(m_Materials.size());

	std::unordered_map<string, int> names;

	u32 matsLoaded;
	for (u32 i=0; i< aiscene->mNumMaterials; ++i)
	{
		auto* aimat = aiscene->mMaterials[i];
		printf("Material %s\n", aimat->GetName().C_Str());
		//for (u32 p=0; p<aimat->mNumProperties; ++p)
		//{
		//	auto prop = aimat->mProperties[p];
		//	printf("Found property %s with type %d", prop->mKey.C_Str(), prop->mType);
		//	if (prop->mType == aiPTI_String)
		//	{
		//		aiString value;
		//		aimat->Get(prop->mKey.C_Str(), 0, 0, value);
		//		printf(", value %s", value.C_Str());
		//	}
		//	printf("\n");
		//}
		//for (int i=0; i<aiTextureType_TRANSMISSION; +i)
		//{
		//	aiString path;
		//	if (aimat->GetTexture((aiTextureType) i, 0, &path) == aiReturn_SUCCESS)
		//	{
		//		printf("Found texture %d: %s\n", i, path.C_Str());
		//	}
		//}
		
		aiColor3D diff;
		aimat->Get(AI_MATKEY_COLOR_DIFFUSE, diff);
		auto& material = m_Materials.emplace_back(std::make_shared<Material>(aimat->GetName().C_Str(), Convert<decltype(Material::colour)>(diff)));
		float opacity;
		if (aimat->Get(AI_MATKEY_OPACITY, opacity) == aiReturn_SUCCESS)
		{
			printf("Opacity %f\n", opacity);
			material->colour.a = opacity;
		}
		else
		{
			material->colour.a = 1;
		}

		int blend;
		if (aimat->Get(AI_MATKEY_BLEND_FUNC, blend) == aiReturn_SUCCESS)
		{
			printf("Blend func %d\n", blend);
		}
		aiString gltfAlphaMode;
		if (aimat->Get(AI_MATKEY_GLTF_ALPHAMODE, gltfAlphaMode) == aiReturn_SUCCESS)
		{
			printf("Alpha Mode %s\n", gltfAlphaMode.C_Str());
			if (strcmp(gltfAlphaMode.C_Str(), "BLEND") == 0)
			{
				material->translucent = true;
			}
			else if (strcmp(gltfAlphaMode.C_Str(), "MASK") == 0)
			{
				aimat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, material->mask);
				printf("Material mask: %f\n", material->mask);
			}
			else if (strcmp(gltfAlphaMode.C_Str(), "OPAQUE") != 0)
			{
				printf("Unhandled alpha mode %s\n", gltfAlphaMode.C_Str());
			}
		}

		if (loadTextures)
		{
			MatLoadingJob job;
			job.m_Mat = material;
			job.m_Import = isImport;
			job.m_MeshPath = importPath;
			job.m_Albedo = GetAndUpdateTexture(aimat, aiTextureType_DIFFUSE, isImport);
			job.m_Normal = GetAndUpdateTexture(aimat, aiTextureType_NORMALS, isImport);
			job.m_Emissive = GetAndUpdateTexture(aimat, aiTextureType_EMISSIVE, isImport);
			job.m_Roughness = GetAndUpdateTexture(aimat, aiTextureType_DIFFUSE_ROUGHNESS, isImport);
			if (!job.m_Albedo.empty() && material->DebugName.size() < 4)
			{
				material->DebugName = std::filesystem::path(job.m_Albedo).filename().string();
			}

			if (async)
			{
				m_LoadingJobs.push(job);
			}
			else
			{
				Perform(job);
			}

			//if (!LoadMatTexture(aimat, aiTextureType_DIFFUSE, material.albedo, importPath, isImport))
			//{
			//	//material.albedo = 
			//}
			//if (LoadMatTexture(aimat, aiTextureType_NORMALS, material.normal, importPath, isImport))
			//{
			//	printf("Loaded normal map\n");
			//}
			//if (LoadMatTexture(aimat, aiTextureType_EMISSIVE, material.emissive, importPath, isImport))
			//{
			//	printf("Loaded emissive map\n");
			//}
			//if (LoadMatTexture(aimat, aiTextureType_DIFFUSE_ROUGHNESS, material.roughnessMap, importPath, isImport))
			//{
			//	printf("Loaded roughness map\n");
			//}
		}
		matsLoaded = i;
	}
	printf("Loaded %d materials\n", matsLoaded);

//	u32 meshStart = m_Meshes.size();
	Vector<MeshPart> meshParts;
	for (u32 i=0; i< aiscene->mNumMeshes; ++i)
	{
		auto* mesh = aiscene->mMeshes[i];
		u32 vcount = mesh->mNumVertices;
		vector<Vertex> verts(vcount);
		for (u32 i=0; i<vcount; ++i)
		{
			verts[i].pos = Vec3(mesh->mVertices[i]);
			assert(mesh->mNormals != nullptr);
			if (nullptr != mesh->mNormals)
			{
				verts[i].normal = Vec3(mesh->mNormals[i]);
			}
			if (nullptr != mesh->mTangents)
			{
				verts[i].tangent = Vec3(mesh->mTangents[i]);
			}
			if (nullptr != mesh->mTextureCoords[0])
			{
				verts[i].uvs = Vec2(mesh->mTextureCoords[0][i]);
			}
		}

		u32 fcount = mesh->mNumFaces;
		vector<IndexedTri> faces(fcount);
		for (u32 i=0; i<fcount; ++i)
		{
			auto* inds = mesh->mFaces[i].mIndices;
			faces[i] = { u16(inds[0]),
				u16(inds[1]),
				u16(inds[2]) };
		}
		//mesh->
		string partName(mesh->mName.C_Str());
		if (names[partName]++ != 0)
		{
			partName += std::format("[{}]", names[partName] - 1);
		}
		printf("Loaded mesh part %s\n", partName.c_str());
		AssetPath smPath = path;
		if (aiscene->mNumMeshes == 1)
		{
			smPath.SubPath = partName;
		}
		meshParts.emplace_back(verts, faces, matsLoaded > 0 ? mesh->mMaterialIndex + matStart : 0, smPath);
		
	}
//	printf("Added %d meshes\n", int(m_Meshes.size() - meshStart));
	

	SceneletLoadCtx ctx { Scenelet{path}};
	//Scenelet& scenelet = m_Scenelets.emplace_back(path);
	Scenelet& scenelet = ctx.m_Scenelet;
	AddNode(ctx, scenelet.m_Root, meshParts, root, aiMatrix4x4{});
	if (scenelet.m_Root.children.size() == 1)
	{
		SceneletPart temp = std::move(scenelet.m_Root.children[0]);
		scenelet.m_Root = std::move(temp);
		//if (scenelet.m_Root.children.empty())
		//{
		//	auto name = scenelet.m_Root.mesh->GetPath();
		//	scenelet.m_Root.mesh->SetPath(name.substr(0, name.find(':')));
		//}
	}
	if (async)
	{
		m_LoadedScenelets.push(std::move(ctx));
	}
	else
	{
		FinishLoad(ctx);
	}
	return static_cast<s32>(m_Scenelets.size() - 1);
}

CompoundMesh::Ref AssetManager::AddMesh(CompoundMesh&& mesh)
{
	auto& result = m_CompoundMeshes.emplace_back(std::make_shared<CompoundMesh>(std::move(mesh)));
	m_LoadedMeshes[result->GetPath()] = result;
	return result;
	//return CompoundMesh::Ref{ static_cast<s32>(m_CompoundMeshes.size() - 1) };
}

 AssetManager::AssetManager()
{
	RefreshMeshList();
}

AssetManager::~AssetManager()
{
	if (m_WorkerThread != nullptr)
	{
		Finish();
	}
}

//Asset::Ref AssetManager::LoadAssetUntyped(AssetPath path)
//{
//	if (path.ends_with(".jpg") || path.ends_with(".png"))
//	{
//	}
//	else if (path.ends_with(".obj") || path.ends_with(".glb"))
//	{
//		return ImportScenelet(path);
//	}
//	return -1;
//}

//Asset::Ref AssetManager::LoadAssetUntyped(std::filesystem::directory_entry const& file)
//{
//	return LoadAssetUntyped(ToString(file.path().filename()));
//}

Scenelet::Ref AssetManager::ImportScenelet(String const& path, bool loadTextures, bool async)
{
	static const String meshDir = "meshes/";
	fs::path filePath = path;
	Assimp::Importer importer;
	auto flags = aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded
			  | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
			  | aiProcess_FlipWindingOrder;
	if (m_PreTransform) flags |= aiProcess_PreTransformVertices;

	const aiScene*	 aiscene = importer.ReadFile(GetFilePath(filePath), flags);
	if (aiscene == nullptr)
	{
		fprintf(stderr, "Error: %s", importer.GetErrorString());
	}

	cout << path << endl;

	if (aiscene != nullptr)
	{
		u32 start = NumCast<u32>(m_Meshes.size());
		auto assName = filePath.filename();
		AssetPath assetPath("/Content/" + meshDir + assName.replace_extension().string());
		u32 end = AddScenelet(aiscene, assetPath, true, path, loadTextures, async);

		for (u32 i=0; i<aiscene->mNumMaterials; ++i)
		{
			
			//auto& mat = aiscene->mMaterials[i];
			//fs::path					old = mat->GetName().C_Str();
			//
			//aiString newName = aiString {ToString("textures" / old.filename())};
			//mat->AddProperty(&newName, AI_MATKEY_NAME);
		}
		fs::path outDir = meshDir;
		assName.replace_extension();
		fs::path assPath = (outDir / assName).replace_extension("assbin");
		//auto outDir = assetDir / assPath.parent_path();
		fs::create_directories(outDir);
//		assPath += ".assbin";
		String outFile = ToString(assPath);

		Assimp::Exporter exporter;
		auto retval = exporter.Export(aiscene, "assbin", outFile);
		if (retval != aiReturn_SUCCESS)
		{
			printf("Export error: %s", exporter.GetErrorString());
		}
		return end;
	}
	else
	{
		fprintf(stderr, "Failed to import mesh %s: %s\n", path.c_str(), importer.GetErrorString());
		return -1;
	}

}

void AssetManager::ImportSceneletAsync(String const& path, bool loadTextures /*= true*/, AssetLoadCallback callback)
{
	m_LoadingJobs.push(SceneletLoadingJob{  path, true, loadTextures, callback});
}

//CompoundMesh::Ref AssetManager::LoadMesh(AssetPath const& path, bool loadTextures /*= true*/)
//{
//}

Scenelet::Ref AssetManager::LoadScenelet(AssetPath const& path, bool loadTextures /*= true*/, bool async)
{
	String subMesh = path.SubPath;
	String filePath = path.ToFilePath();
	fs::path assetPath = "assets";
	assetPath /= filePath;
	assetPath += ".assbin";
	Assimp::Importer importer;
	//auto flags = aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded
	//		  | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
	//		  | aiProcess_FlipWindingOrder;
//	if (m_PreTransform) flags |= aiProcess_PreTransformVertices;
	const aiScene* aiscene = importer.ReadFile(assetPath.string(), aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
	if (aiscene == nullptr)
	{
		fprintf(stderr, "Error: %s", importer.GetErrorString());
	}
	cout << path.ToString() << endl;
	Assimp::Exporter exporter;
	//exporter.Export(aiscene, )
	//for (u32 i = 0; i < exporter.GetExportFormatCount(); ++i)
	//{
	//	auto desc = exporter.GetExportFormatDescription(i);
	//	printf("Available format %s (.%s): %s\n", desc->id, desc->fileExtension, desc->description);
	//}

	if (aiscene != nullptr)
	{
		u32 start = NumCast<u32>(m_Meshes.size());
		u32 end = AddScenelet(aiscene, path.GetAssetRootPath(), false, "", loadTextures, async);
		if (!subMesh.empty())
		{
			for (u32 i=start; i< m_Meshes.size(); ++i)
			{
				if (m_Meshes[i].name == subMesh)
				{
					return i;
				}
			}
			std::cerr << "Failed to find submesh " << subMesh << endl;
		}
		return end;
	}
	else
	{
		std::cerr << "Failed to load mesh " << path.ToString() << endl;
		return -1;
	}
}

void AssetManager::LoadSceneletAsync(AssetPath const& path, bool loadTextures /*= true*/, AssetLoadCallback callback /*= nullptr*/)
{
	m_LoadingJobs.push(SceneletLoadingJob{ path.PrimaryPath, false, loadTextures, callback });
}

CompoundMesh::Ref AssetManager::GetMesh(AssetPath const& path)
{
	if (auto loadedIt = m_LoadedMeshes.find(path); loadedIt != m_LoadedMeshes.end())
	{
		return loadedIt->second;
	}
	String subMesh = path.SubPath;
	auto sceneletPath = path.GetAssetRootPath();
	auto sl = FindScenelet(sceneletPath);
	if (!IsValid(sl))
	{
		sl = LoadScenelet(sceneletPath);
	}

	if (!IsValid(sl))
	{
		return CompoundMesh::INVALID_REF;
	}

	return m_Scenelets[sl].FindMesh(subMesh);

}

MeshRef AssetManager::FindMesh(AssetPath const& path)
{
	int i=0;
	for (auto& mesh : m_Meshes)
	{
		if (mesh.m_Path == path)
		{
			return i;
		}
		++i;
	}
	return -1;
}

void AssetManager::RefreshMeshList()
{
	m_AllMeshes.clear();
	for (auto const& dir_entry : fs::directory_iterator("assets/meshes"))
	{
		auto assPath = AssetPath::Content(ToString(dir_entry.path().lexically_relative("assets").replace_extension()));
		m_AllMeshes.emplace_back(AssetData { assPath, IsValid(FindMesh(assPath)) });
	}
}

String AssetManager::GetFilePath(std::filesystem::path const& path)
{
//	if (path.is_absolute())
	{
		return ToString(path);
	}
//	return "content" / path;
//	return ToString(path);
}

void AssetManager::DrawControls()
{
	static bool loadTextures = true;
	if (ImGui::Button("Import mesh"))
	{
		nfdchar_t *outPath;
		
		auto contentDir = (std::filesystem::current_path() / "assets");
		printf("Opening file dialog at %s\n", contentDir.string().c_str());
		nfdresult_t result = NFD_OpenDialog(NULL, contentDir.string().c_str(), &outPath);
		if (result == NFD_OKAY)
		{
			std::filesystem::path asset(outPath);
			auto relPath = ToString(asset);
			printf("Loading mesh %s\n", relPath.c_str());
			ImportSceneletAsync(relPath, loadTextures);
			free(outPath);
		}
		else if (result == NFD_ERROR)
		{
			printf("Error: %s\n", NFD_GetError());
		}
	}
	ImGui::Checkbox("Load textures", &loadTextures);
	ImGui::Checkbox("Pretransform", &m_PreTransform);

}

void AssetManager::Start()
{
	RASSERT(m_WorkerThread == nullptr);
	m_WorkerThread = std::make_unique<std::thread>([this] { Work(); });
}

void AssetManager::Finish()
{
	RASSERT(m_WorkerThread != nullptr && m_WorkerThread->joinable());
	m_LoadingJobs.push(FinishJob{});
	m_WorkerThread->join();
	m_WorkerThread = nullptr;
}

void AssetManager::Synchronize()
{
	while (!m_LoadedScenelets.IsEmpty())
	{
		SceneletLoadCtx ctx = m_LoadedScenelets.pop();
		printf("Added scenelet %s", ctx.m_Scenelet.GetPath().ToString().c_str());
		FinishLoad(ctx);
	}
}

void AssetManager::Work()
{
	for (;;)
	{
		LoadingJob job = m_LoadingJobs.pop();
		if (std::holds_alternative<FinishJob>(job))
		{
			return;
		}

		if (std::holds_alternative<SceneletLoadingJob>(job))
		{
			SceneletLoadingJob& slJob = std::get<SceneletLoadingJob>(job);
			if (slJob.isImport)
			{
				ImportScenelet(slJob.path, slJob.loadTextures, true);
			}
			else
			{
				LoadScenelet(AssetPath(slJob.path), slJob.loadTextures, true);
			}
		}
		else if (std::holds_alternative<MatLoadingJob>(job))
		{
			MatLoadingJob& mlJob = std::get<MatLoadingJob>(job);
			Perform(mlJob);
		}


	}
}

void AssetManager::Tick()
{
	Synchronize();
}

