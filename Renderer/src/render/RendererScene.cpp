#include "RendererScene.h"
#include "RenderDevice.h"
#include "DeviceMesh.h"

namespace rnd
{

RenderObject* RendererScene::AddPrimitive(SceneDataInterface::SMCreationData const& data)
{
	RenderObject* object = &mPrimitives[data.Id];
	for (u32 i = 0; i < data.Mesh->components.size(); ++i)
	{
		auto& meshPart = data.Mesh->components[i];
		VertexBufferData vbd;
		vbd.Data = Addr(meshPart.vertices);
		vbd.VertSize = sizeof(meshPart.vertices[0]);
		vbd.NumVerts = NumCast<u32>(meshPart.vertices.size());
		vbd.VertAtts = GetVertAttHdl(meshPart.vertices[0]);
		auto batchedUpload = mDevice->StartBatchedUpload();
		object->MeshData.push_back(mDevice->CreateIndexedMesh(EPrimitiveTopology::TRIANGLES, vbd, {&meshPart.triangles[0][0], meshPart.triangles.size() * 3}, batchedUpload));
		object->Materials.push_back(mMaterialMgr.GetOrCreateRenderMaterial(data.Materials[i]));
	}
//	mPrimitives[data.Id].push_back(object);
	return object;
}

void RendererScene::UpdatePrimitives()
{
	auto& sceneData = mDataInterface->GetRenderThreadData();
	for (PrimitiveId id : sceneData.RemovedPrimitives)
	{
		mPrimitives.erase(id);
	}
	
	for (auto const& creation : sceneData.AddedPrimitives)
	{
		AddPrimitive(creation);
	}
}

}
