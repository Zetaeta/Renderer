#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <format>
#include <iostream>
#include <filesystem>

using std::string;
using std::vector;
using std::cout;
using std::endl;

inline vec3 Vec3(aiVector3D const& v)
{
	return {v.x, v.y, v.z};
}

inline vec2 Vec2(aiVector3D const& v)
{
	return {v.x, v.y};
}

void AssetManager::AddNode(std::vector<MeshPart>& components, aiNode const* node, aiMatrix4x4 transform, u32 meshOffset)
{
	printf("Checking node %s with %d meshes, %d children\n", node->mName.C_Str(), node->mNumMeshes, node->mNumChildren);
	for (u32 i=0; i<node->mNumMeshes; ++i)
	{
		u32 mesh = node->mMeshes[i];
		auto trans = node->mTransformation;
		//scene.m_Meshes.emplace_back
		components.emplace_back(MeshPart{meshOffset, std::string(node->mName.C_Str())});
	}
	for (u32 i=0; i<node->mNumChildren; ++i)
	{
		AddNode(components, node->mChildren[i], node->mTransformation, meshOffset);
	}
}


bool AssetManager::LoadTexture(aiMaterial const* aimat, aiTextureType texType, TextureRef& outTexture)
{

	aiString diffusePath;
	if (aimat->GetTexture(texType, 0,&diffusePath) == aiReturn_SUCCESS)
	{
		string file = std::format("content/{} ", diffusePath.C_Str());
		outTexture = Texture::LoadFrom(file.c_str());
		return outTexture->IsValid();
	}
	return false;

}

MeshRef AssetManager::AddMesh(aiScene const* aiscene)
{
	auto* root = aiscene->mRootNode;
	auto& cmesh = m_CompoundMeshes.emplace_back(string(root->mName.C_Str()));

	u32 matStart = m_Materials.size();

	u32 matsLoaded;
	for (u32 i=0; i< aiscene->mNumMaterials; ++i)
	{
		auto* aimat = aiscene->mMaterials[i];
		aiColor3D diff;
		aimat->Get(AI_MATKEY_COLOR_DIFFUSE, diff);
		auto& material = m_Materials.emplace_back(Convert<decltype(Material::colour)>(diff));
		if (!LoadTexture(aimat, aiTextureType_DIFFUSE, material.albedo))
		{
			//material.albedo = 
		}
		if (LoadTexture(aimat, aiTextureType_NORMALS, material.normal))
		{
			printf("Loaded normal map\n");
		}
		if (LoadTexture(aimat, aiTextureType_EMISSIVE, material.emissive))
		{
			printf("Loaded emissive map\n");
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
		m_Meshes.emplace_back(verts, faces, string(mesh->mName.C_Str()), matsLoaded > 0 ? mesh->mMaterialIndex + matStart : 0);
		
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
		LoadMesh(path);
		return 0;
	}
}

Asset::Ref AssetManager::LoadAssetUntyped(std::filesystem::directory_entry const& file)
{
	return LoadAssetUntyped(file.path().filename().string());
}

MeshRef AssetManager::LoadMesh(AssetPath path)
{

	Assimp::Importer importer;
	const aiScene*	 aiscene = importer.ReadFile(GetFilePath(path),
		  aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded
			  | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
			  | aiProcess_PreTransformVertices | aiProcess_FlipWindingOrder);
	cout << path << endl;
	if (aiscene != nullptr)
	{
		return AddMesh(aiscene);
	}
	else
	{
		return -1;
	}
}

String AssetManager::GetFilePath(AssetPath path)
{
	return "content/" + path;
}

