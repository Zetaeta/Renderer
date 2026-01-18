#pragma once
#include "common/Material.h"
#include "RenderMaterial.h"

namespace rnd
{
class IRenderDevice;
class RenderMaterial;

DECLARE_MATERIAL_SHADER(TexturedMat)
DECLARE_MATERIAL_SHADER(PlainMat)
DECLARE_MATERIAL_SHADER(PointShadow)
DECLARE_MATERIAL_SHADER(ScreenId)
DECLARE_MATERIAL_SHADER(Mat2D)
DECLARE_MATERIAL_SHADER(Mat2DUint)
DECLARE_MATERIAL_SHADER(MatCube)
DECLARE_MATERIAL_SHADER(MatCubeDepth)

class MaterialManager
{
public:
	MaterialManager() {}
	MaterialManager(rnd::IRenderDevice* device);
	~MaterialManager();
	ZE_COPY_PROTECT(MaterialManager)
	RMOVE_DEFAULT(MaterialManager)

	void Release();
	RenderMaterial* GetDefaultMaterial(int matType);

	RenderMaterial* GetOrCreateRenderMaterial(MaterialID mid);

	void UpdateMaterial(MaterialID matId);
	void UpdateMaterial(MaterialID matId, TexturedRenderMaterial* mat);

	const RenderMaterialType& MatType(u32 index)
	{
		return mMatTypes[index];
	}

	std::array<RenderMaterialType, MAT_COUNT>& MatTypes()
	{
		return mMatTypes;
	}
protected:


	std::array<RenderMaterialType, MAT_COUNT> mMatTypes;
	rnd::IRenderDevice* mDevice = nullptr;
	std::unordered_map<MaterialID, RenderMaterial*> mMaterials;

	//std::vector<OwningPtr<MaterialArchetype>> mMatTypes;
	//std::vector<OwningPtr<IDeviceMaterial>> mMaterials;
};

}
