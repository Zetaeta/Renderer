#pragma once
#include "common/Material.h"
#include "../scene/Scene.h"

class IDeviceMaterial;
namespace rnd { class RenderMaterial; }

namespace rnd { class IRenderDevice; }

namespace rnd
{

class MaterialManager
{
public:
	MaterialManager(rnd::IRenderDevice* device)
		: mDevice(device)
	{
	}
	IDeviceMaterial* GetDefaultMaterial(int matType);

	RenderMaterial* GetOrCreateRenderMaterial(MaterialID mid);

	std::unordered_map<MaterialID, RenderMaterial*> mMaterials;

	//std::vector<OwningPtr<MaterialArchetype>> mMatTypes;
	//std::vector<OwningPtr<IDeviceMaterial>> mMaterials;
	rnd::IRenderDevice* mDevice;
};

}
