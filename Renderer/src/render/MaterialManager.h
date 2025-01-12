#pragma once
#include "common/Material.h"
#include "scene/Scene.h"
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
	MaterialManager(rnd::IRenderDevice* device);
	void Release();
	IDeviceMaterial* GetDefaultMaterial(int matType);

	RenderMaterial* GetOrCreateRenderMaterial(MaterialID mid);

	std::unordered_map<MaterialID, RenderMaterial*> mMaterials;

	const RenderMaterialType& MatType(u32 index)
	{
		return mMatTypes[index];
	}

	std::array<RenderMaterialType, MAT_COUNT>& MatTypes()
	{
		return mMatTypes;
	}
private:
	std::array<RenderMaterialType, MAT_COUNT> mMatTypes;

	//std::vector<OwningPtr<MaterialArchetype>> mMatTypes;
	//std::vector<OwningPtr<IDeviceMaterial>> mMaterials;
	rnd::IRenderDevice* mDevice;
};

}
