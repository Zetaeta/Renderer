#include "MaterialManager.h"
#include "dx11/DX11Renderer.h"

namespace rnd
{


DEFINE_MATERIAL_SHADER(TexturedMat, "TexPixelShader", "main", "TexVertexShader", "main", VA_Position | VA_Normal | VA_Tangent | VA_TexCoord);
DEFINE_MATERIAL_SHADER(PlainMat, "PlainPixelShader", "main", "PlainVertexShader", "main", VA_Position | VA_Normal | VA_Tangent | VA_TexCoord);
DEFINE_MATERIAL_SHADER(PointShadow, "PointShadow_PS", "main", "PointShadow_VS", "main", VA_Position | VA_Normal | VA_Tangent | VA_TexCoord);
DEFINE_MATERIAL_SHADER(ScreenId, "ScreenId_PS", "main", "ScreenId_VS", "main", VA_Position | VA_TexCoord);
DEFINE_MATERIAL_SHADER(Mat2D, "2D_PS", "main", "2D_VS", "main", VA_Position | VA_TexCoord);
DEFINE_MATERIAL_SHADER(Mat2DUint, "2D_PS_Uint", "main", "2D_VS", "main", VA_Position | VA_TexCoord);
DEFINE_MATERIAL_SHADER(MatCube, "BGPixelShader", "main", "BGVertexShader", "main", VA_Position | VA_TexCoord);
DEFINE_MATERIAL_SHADER(MatCubeDepth, "DepthCube_PS", "main", "BGVertexShader", "main", VA_Position | VA_TexCoord);

rnd::RenderMaterial* rnd::MaterialManager::GetOrCreateRenderMaterial(MaterialID mid)
{
	Material&  mat = AssetManager::Get()->GetMaterial(mid);
	RenderMaterial*& rmat = mMaterials[mid];
	bool needsUpdate = false;
	if (rmat == nullptr)
	{
		MaterialArchetype* archetype = nullptr;
		if (mat.IsTextured)
		{
			rmat = new TexturedRenderMaterial(&mMatTypes[MAT_TEX], mat.GetMatType(), mat.Props);
			UpdateMaterial(mid, static_cast<TexturedRenderMaterial*>(rmat));
		}
		else
		{
			rmat = new RenderMaterial(&mMatTypes[MAT_PLAIN], mat.GetMatType(), mat.Props);
		}
	}
	return rmat;
}

MaterialManager::MaterialManager(rnd::IRenderDevice* device) : mDevice(device)
{
	mMatTypes[MAT_PLAIN] = MaterialArchetype(PlainMat);
	mMatTypes[MAT_TEX] = MaterialArchetype(TexturedMat);
	mMatTypes[MAT_POINT_SHADOW_DEPTH] = MaterialArchetype(PointShadow);
	mMatTypes[MAT_SCREEN_ID] = MaterialArchetype(ScreenId);
	mMatTypes[MAT_2D] = MaterialArchetype(Mat2D);
	mMatTypes[MAT_2D_UINT] = MaterialArchetype(Mat2DUint);
	mMatTypes[MAT_BG] = MaterialArchetype(MatCube);
	mMatTypes[MAT_CUBE_DEPTH] = MaterialArchetype(MatCubeDepth);

	static DataLayout screenIdLayout(16, { { &GetTypeInfo<u32>(), "screenObjectId", 0 } });
	mMatTypes[MAT_SCREEN_ID].CBData[Denum(ECBFrequency::PS_PerInstance)].Layout = &screenIdLayout;
}

MaterialManager::~MaterialManager()
{
	Release();
}

void MaterialManager::Release()
{
	for (auto& [id, mat] : mMaterials)
	{
		if (mat)
		{
			mat->Release();
			delete mat;
		}
	}
	mMaterials.clear();
	mMatTypes = {};
}

RenderMaterial* MaterialManager::GetDefaultMaterial(int matType)
{
	auto& materialType = MatTypes()[matType];
	if (!IsValid(materialType.m_Default))
	{
		materialType.m_Default = std::make_unique<RenderMaterial>(&materialType, E_MT_OPAQUE, StandardMatProperties{});
		
	}

	return materialType.m_Default.get();
}


void MaterialManager::UpdateMaterial(MaterialID matId)
{
	if (TexturedRenderMaterial* texMat = dynamic_cast<TexturedRenderMaterial*>(mMaterials[matId]))
	{
		UpdateMaterial(matId, texMat);
	}
}

void MaterialManager::UpdateMaterial(MaterialID matId, TexturedRenderMaterial* texMat)
{
	Material const& mat = AssetManager::Get()->GetMaterial(matId);
		if (mat.albedo.IsValid())
		{
			texMat->m_Albedo = mDevice->ResourceMgr.GetDeviceTexture(&*mat.albedo);
		}
		if (mat.normal.IsValid())
		{
			texMat->m_Normal = mDevice->ResourceMgr.GetDeviceTexture(&*mat.normal);
//			mat.normal.Clear();
		}
		if (mat.emissiveMap.IsValid())
		{
			texMat->m_Emissive = mDevice->ResourceMgr.GetDeviceTexture(&*mat.emissiveMap);
//			mat.emissive.Clear();
		}
		else
		{
			texMat->m_Emissive = mDevice->BasicTextures.GetBlackTexture(mDevice);
		}
		if (mat.roughnessMap.IsValid())
		{
			texMat->m_Roughness = mDevice->ResourceMgr.GetDeviceTexture(&*mat.roughnessMap);
		}
		else
		{
			texMat->m_Roughness = nullptr; mDevice->BasicTextures.GetBlackTexture(mDevice);
		}
}

}
