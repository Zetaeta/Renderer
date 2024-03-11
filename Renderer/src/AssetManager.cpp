#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/pbrmaterial.h>
#include <format>
#include <iostream>
#include <filesystem>
#include "Utils.h"
#include "imgui.h"
#include <nfd.h>

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
	constexpr float R2D =  180 / M_PI;
	_rot.yaw = rot.y * R2D;
	_rot.pitch = rot.x * R2D;
	_rot.roll = rot.z * R2D;
	return RotTransform(_pos, _scale, _rot);
}

void AssetManager::AddNode(std::vector<MeshPart>& components, aiNode const* node, aiMatrix4x4 transform, u32 meshOffset)
{
	printf("Checking node %s with %d meshes, %d children\n", node->mName.C_Str(), node->mNumMeshes, node->mNumChildren);
	auto totalTrans =  node->mTransformation * transform;
	auto trans = Mat2Trans( totalTrans);
	for (u32 i=0; i<node->mNumMeshes; ++i)
	{
		u32 mesh = node->mMeshes[i];
		//scene.m_Meshes.emplace_back
		u32 meshInd = meshOffset + mesh;
		auto& meshName = m_Meshes[meshInd].name;
		u32	  sep = meshName.find(':');
		if (sep < meshName.size() && (meshName.size() == sep + 1 || meshName[sep + 1] == '['))
		{
			String subName = std::format("{}-{}", node->mName.C_Str(), i);
			meshName.replace(sep + 1, meshName.size() - (sep + 1), subName.c_str());
			m_Meshes[meshInd].m_Path = meshName;
			printf("Set mesh name to %s\n", meshName.c_str());
		}
		//if (m_Meshes[meshInd].name.empty())
		//{
		//}
		components.emplace_back(MeshPart{meshInd, std::string(node->mName.C_Str()), trans});
	}
	for (u32 i=0; i<node->mNumChildren; ++i)
	{
		AddNode(components, node->mChildren[i], totalTrans, meshOffset);
	}
}

bool AssetManager::LoadMatTexture(aiMaterial* aimat, aiTextureType texType, TextureRef& outTexture, AssetPath const& meshPath, bool isImport)
{
	//String meshDir = "content/" 
	aiString diffusePath;
	if (aimat->GetTexture(texType, 0,&diffusePath) == aiReturn_SUCCESS)
	{
		printf("Loading texture %s\n", diffusePath.C_Str());
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
			fs::path textureDirs[] = { "content", "content/textures", meshDir, meshDir / "textures" };
			for (fs::path const& dir : textureDirs)
			{
				auto path = dir / diffusePath.C_Str();
				if (fs::exists(path))
				{
					outTexture = Texture::LoadFrom(ToString(path).c_str());
					if (outTexture.IsValid())
					{
						fs::path assetPath = "textures";
						assetPath /= path.filename();
						aiString astr { assetPath.string().c_str() };
						aimat->AddProperty(&astr, AI_MATKEY_TEXTURE(texType, 0));
						m_LoadedTextures[ToString(assetPath)] = outTexture;
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
						return true;
					}
					printf("Failed to import texture %s\n", ToString(path).c_str());
				}
			}
			fprintf(stderr, "Failed to find texture!\n");
		}
		else
		{
			auto res = m_LoadedTextures.find(String(diffusePath.C_Str()));
			if (res != m_LoadedTextures.end())
			{
				outTexture = res->second;
				printf("Already loaded texture %s", diffusePath.C_Str());
				return true;
			}
			fs::path path = "assets";
			path /= diffusePath.C_Str();
			outTexture = Texture::LoadFrom(ToString(path).c_str());
			if (!outTexture.IsValid())
			{
				printf("Failed to load texture %s\n", ToString(path).c_str());
				return false;
			}
			return true;
		}
		return false;
		//string file = std::format("content/{} ", diffusePath.C_Str());
		//outTexture = Texture::LoadFrom(file.c_str());
		//return outTexture->IsValid();
	}
	return false;

}

MeshRef AssetManager::AddMesh(aiScene const* aiscene, AssetPath const& path, bool isImport, bool loadTextures /*= true*/ )
{
	auto* root = aiscene->mRootNode;
	auto& cmesh = m_CompoundMeshes.emplace_back(string(root->mName.C_Str()));

	u32 matStart = m_Materials.size();

	std::unordered_map<string, int> names;

	u32 matsLoaded;
	for (u32 i=0; i< aiscene->mNumMaterials; ++i)
	{
		auto* aimat = aiscene->mMaterials[i];
		printf("Material %s\n", aimat->GetName().C_Str());
		for (u32 p=0; p<aimat->mNumProperties; ++p)
		{
			auto prop = aimat->mProperties[p];
			printf("Found property %s with type %d", prop->mKey.C_Str(), prop->mType);
			if (prop->mType == aiPTI_String)
			{
				aiString value;
				aimat->Get(prop->mKey.C_Str(), 0, 0, value);
				printf(", value %s", value.C_Str());
			}
			printf("\n");
		}
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
		auto& material = m_Materials.emplace_back(Convert<decltype(Material::colour)>(diff));
		float opacity;
		if (aimat->Get(AI_MATKEY_OPACITY, diff) == aiReturn_SUCCESS)
		{
			printf("Opacity %f\n", opacity);
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
				material.translucent = true;
			}
		}

		if (loadTextures)
		{
			if (!LoadMatTexture(aimat, aiTextureType_DIFFUSE, material.albedo, path, isImport))
			{
				//material.albedo = 
			}
			if (LoadMatTexture(aimat, aiTextureType_NORMALS, material.normal, path, isImport))
			{
				printf("Loaded normal map\n");
			}
			if (LoadMatTexture(aimat, aiTextureType_EMISSIVE, material.emissive, path, isImport))
			{
				printf("Loaded emissive map\n");
			}
			if (LoadMatTexture(aimat, aiTextureType_DIFFUSE_ROUGHNESS, material.roughnessMap, path, isImport))
			{
				printf("Loaded roughness map\n");
			}
		}
		matsLoaded = i;
	}
	printf("Loaded %d materials\n", matsLoaded);

	u32 meshStart = m_Meshes.size();
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
		AssetPath smPath = aiscene->mNumMeshes == 1 ? path : (path + ":" + partName);
		m_Meshes.emplace_back(smPath, verts, faces, smPath, matsLoaded > 0 ? mesh->mMaterialIndex + matStart : 0);
		
	}
	printf("Added %d meshes\n", int(m_Meshes.size() - meshStart));
	

	AddNode(cmesh.components, root, aiMatrix4x4{}, meshStart);
	return static_cast<s32>(m_Meshes.size() - 1);
}

MeshRef AssetManager::AddMesh(Mesh&& mesh)
{
	m_Meshes.emplace_back(mesh);
	return MeshRef{ static_cast<s32>(m_Meshes.size() - 1) };
}

Asset::Ref AssetManager::LoadAssetUntyped(AssetPath path)
{
	if (path.ends_with(".jpg") || path.ends_with(".png"))
	{
	}
	else if (path.ends_with(".obj") || path.ends_with(".glb"))
	{
		ImportMesh(path);
		return 0;
	}
}

Asset::Ref AssetManager::LoadAssetUntyped(std::filesystem::directory_entry const& file)
{
	return LoadAssetUntyped(ToString(file.path().filename()));
}

MeshRef AssetManager::ImportMesh(String const& path, bool loadTextures)
{
	fs::path filePath = path;
	Assimp::Importer importer;
	auto flags = aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded
			  | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
			  | aiProcess_FlipWindingOrder;
	if (m_PreTransform) flags |= aiProcess_PreTransformVertices;
	const aiScene*	 aiscene = importer.ReadFile(GetFilePath(filePath), flags);
	if (aiscene == nullptr)
	{
		fprintf(stderr, "Error: ", importer.GetErrorString());
	}
	cout << path << endl;
	Assimp::Exporter exporter;
	//exporter.Export(aiscene, )
	//for (u32 i = 0; i < exporter.GetExportFormatCount(); ++i)
	//{
	//	auto desc = exporter.GetExportFormatDescription(i);
	//	printf("Available format %s (.%s): %s\n", desc->id, desc->fileExtension, desc->description);
	//}

	if (aiscene != nullptr)
	{
		u32 start = m_Meshes.size();
		u32 end = AddMesh(aiscene, path, loadTextures, true);

		for (int i=0; i<aiscene->mNumMaterials; ++i)
		{
			
			//auto& mat = aiscene->mMaterials[i];
			//fs::path					old = mat->GetName().C_Str();
			//
			//aiString newName = aiString {ToString("textures" / old.filename())};
			//mat->AddProperty(&newName, AI_MATKEY_NAME);
		}
		fs::path outDir = "assets/meshes";
		auto assName = filePath.filename();
		assName.replace_extension();
		fs::path assPath = (outDir / assName).replace_extension("assbin");
		//auto outDir = assetDir / assPath.parent_path();
		fs::create_directories(outDir);
//		assPath += ".assbin";
		String outFile = ToString(assPath);
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

void AssetManager::ImportMeshAsync(String const& path, bool loadTextures /*= true*/, AssetLoadCallback callback)
{
	m_LoadingJobs.push(LoadingJob{ EAssetType::MESH, path, true, loadTextures, callback});
}

MeshRef AssetManager::LoadMesh(AssetPath const& path, bool loadTextures /*= true*/)
{
	u32	sep = path.find(':');
	String subMesh;
	String filePath;
	if (sep < path.size())
	{
		subMesh = path.substr(sep+1);
		filePath = path.substr(0, sep);
	}
	else
	{
		filePath = path;
	}
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
		fprintf(stderr, "Error: ", importer.GetErrorString());
	}
	cout << path << endl;
	Assimp::Exporter exporter;
	//exporter.Export(aiscene, )
	//for (u32 i = 0; i < exporter.GetExportFormatCount(); ++i)
	//{
	//	auto desc = exporter.GetExportFormatDescription(i);
	//	printf("Available format %s (.%s): %s\n", desc->id, desc->fileExtension, desc->description);
	//}

	if (aiscene != nullptr)
	{
		u32 start = m_Meshes.size();
		u32 end = AddMesh(aiscene, filePath, false, loadTextures);
		if (!subMesh.empty())
		{
			for (u32 i=start; i<= end; ++i)
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
		std::cerr << "Failed to load mesh " << path << endl;
		return -1;
	}
}

void AssetManager::LoadMeshAsync(AssetPath const& path, bool loadTextures /*= true*/, AssetLoadCallback callback /*= nullptr*/)
{
	m_LoadingJobs.push(LoadingJob{ EAssetType::MESH, path, false, loadTextures, callback});
}

MeshRef AssetManager::GetMesh(AssetPath path)
{
	auto res = FindMesh(path);
	if (IsValid(res))
	{
		return res;
	}
	return LoadMesh(path);
}

MeshRef AssetManager::FindMesh(AssetPath path)
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
		auto assPath = ToString(dir_entry.path().lexically_relative("assets").replace_extension());
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
		
		auto contentDir = (std::filesystem::current_path() / "content");
		printf("Opening file dialog at %s\n", contentDir.string().c_str());
		nfdresult_t result = NFD_OpenDialog(NULL, contentDir.string().c_str(), &outPath);
		if (result == NFD_OKAY)
		{
			std::filesystem::path asset(outPath);
			auto relPath = ToString(asset);
			printf("Loading mesh %s\n", relPath.c_str());
			ImportMesh(relPath, loadTextures);
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

