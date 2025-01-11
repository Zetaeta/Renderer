#include "MaterialManager.h"
#include "dx11/DX11Renderer.h"

namespace rnd
{

rnd::RenderMaterial* rnd::MaterialManager::GetOrCreateRenderMaterial(MaterialID mid)
{
	Material&  mat = AssetManager::Get()->GetMaterial(mid);
	RenderMaterial* rmat = mMaterials[mid];
	bool needsUpdate = mat.NeedsUpdate();
	if (rmat == nullptr)
	{
		if (mat.IsTextured)
		{
//			MaterialArchetype* archetype = 
		}
//		rmat = mat.IsTextured ? new TexturedRenderMaterial()
	}
	if (mat.NeedsUpdate())
	{

	}
	return nullptr;
}

IDeviceMaterial* MaterialManager::GetDefaultMaterial(int matType)
{
	return static_cast<rnd::dx11::DX11Renderer*>(mDevice)->GetDefaultMaterial(matType);
}


}
