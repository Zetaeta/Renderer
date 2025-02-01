#include "RendererScene.h"
#include "RenderDevice.h"
#include "DeviceMesh.h"
#include "RenderDeviceCtx.h"

namespace rnd
{

static std::unordered_map<Scene const*, std::unordered_map<IRenderDevice*, std::unique_ptr<RendererScene>>> sScenes;

RendererScene* RendererScene::Get(Scene const& scene, IRenderDeviceCtx* deviceCtx)
{
	IRenderDevice* device = deviceCtx->Device;
	auto& result = sScenes[&scene][device];
	if (result == nullptr)
	{
		result = std::make_unique<RendererScene>(device, scene);
	}
	else if (!result->mInitialized)
	{
		*result = RendererScene(device, scene);
	}
	return result.get();
}

void RendererScene::DestroyScene(Scene const& scene)
{
	if (auto it = sScenes.find(&scene); it != sScenes.end())
	{
		for (auto& [_, renderScene] : it->second)
		{
			renderScene->Destroy();
		}

//		sScenes.erase(it);
	}

}

void RendererScene::InitializeScene(Scene const& scene)
{
	if (auto it = sScenes.find(&scene); it != sScenes.end())
	{
		for (auto& [device, renderScene] : it->second)
		{
			*renderScene = RendererScene(device, scene);
		}
	}
}

void RendererScene::OnShutdownDevice(IRenderDevice* device)
{
	for (auto& [_, map] : sScenes)
	{
		map.erase(device);
	}
}

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
		object->Materials.push_back(mDevice->MatMgr.GetOrCreateRenderMaterial(data.Materials[i]));
	}
	object->ScreenId = data.ScreenId;
//	mPrimitives[data.Id].push_back(object);
	return object;
}

void RendererScene::Destroy()
{
	mInitialized = false;
	mPrimitives.clear();
	mScene = nullptr;
	mDataInterface = nullptr;
}

RendererScene::~RendererScene()
{
	Destroy();
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

void RendererScene::BeginFrame()
{
//	mDataInterface->BeginFrame_RenderThread();
	UpdatePrimitives();
}

void RendererScene::EndFrame()
{
//	mDataInterface->EndFrame_RenderThread();
}

RendererScene::RendererScene(IRenderDevice* device, Scene const& scene)
:mDevice(device), mDataInterface(&scene.DataInterface()), mScene(&scene)
{
	mInitialized = true;
}

RendererScene::RendererScene()
{

}

}
