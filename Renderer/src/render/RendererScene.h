#pragma once
#include "common/SceneDataInterface.h"
#include "asset/Mesh.h"
#include "RenderMaterial.h"
#include "MaterialManager.h"

namespace rnd
{
class IRenderDevice;
class IDeviceIndexedMesh;

struct RenderObject
{
	SmallVector<RefPtr<IDeviceIndexedMesh>, 1> MeshData;
	SmallVector<RefPtr<RenderMaterial>, 1> Materials;
};

/**
 * Responsible for scene data used by the render thread(s).
 * In case of multiple render backends, each has its own scene.
 */
class RendererScene
{
	RendererScene(IRenderDevice* device, SceneDataInterface* sceneData)
	:mDevice(device), mDataInterface(sceneData), mMaterialMgr(device) { }
//	Vector<RenderObject*> mPrimitives;
	std::unordered_map<PrimitiveId, RenderObject> mPrimitives;
//	std::unordered_map<MaterialID, RenderMaterial> mMaterials;

	RenderObject* AddPrimitive(SceneDataInterface::SMCreationData const& data);

	void UpdatePrimitives();

	IRenderDevice* mDevice = nullptr;
	SceneDataInterface* mDataInterface = nullptr;

	MaterialManager mMaterialMgr;
};

}
