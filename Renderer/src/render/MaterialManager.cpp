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
	RenderMaterial* rmat = mMaterials[mid];
	bool needsUpdate = mat.NeedsUpdate();
	if (rmat == nullptr)
	{
		MaterialArchetype* archetype = nullptr;
		if (mat.IsTextured)
		{
			rmat = new TexturedRenderMaterial(&mMatTypes[MAT_TEX], mat.GetMatType());
//			MaterialArchetype* archetype = 
		}
		else
		{
			rmat = new RenderMaterial(&mMatTypes[MAT_PLAIN], mat.GetMatType());
		}
//		rmat = mat.IsTextured ? new TexturedRenderMaterial()
	}
	if (mat.NeedsUpdate())
	{

	}
	return nullptr;
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

void MaterialManager::Release()
{
	mMatTypes = {};
}

IDeviceMaterial* MaterialManager::GetDefaultMaterial(int matType)
{
	return static_cast<rnd::dx11::DX11Renderer*>(mDevice)->GetDefaultMaterial(matType);
}


}
