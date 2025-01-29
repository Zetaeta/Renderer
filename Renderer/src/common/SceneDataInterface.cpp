#include "SceneDataInterface.h"
#include "scene/StaticMeshComponent.h"

static Vector<SceneDataInterface*> sInterfaces;
std::mutex sInterfacesMtx;

SceneDataInterface::SceneDataInterface()
:mFrameGuards{std::binary_semaphore(1), std::binary_semaphore(1)}
{
	sInterfaces.push_back(this);
}

 SceneDataInterface::~SceneDataInterface()
{
	Remove(sInterfaces, this);
}

Span<SceneDataInterface*> SceneDataInterface::GetAll()
{
	return sInterfaces;
}

PrimitiveId SceneDataInterface::AddPrimitive(PrimitiveComponent* component)
{
	StaticMeshComponent* smc = Cast<StaticMeshComponent>(component);
	if (smc == nullptr)
	{
		return InvalidPrimId();
	}

	auto& data = GetMainThreadData();
	if (auto mesh = smc->GetMeshRef())
	{
		PrimitiveId newId = NumCast<PrimitiveId>(data.Transforms.size());
//		data.Meshes.push_back(mesh);
		data.Transforms.push_back(component->GetWorldTransform());
		data.TransformsDirty.push_back(true);
		SMCreationData& creationData = data.AddedPrimitives.emplace_back();
		creationData.ScreenId = component->GetScreenId();
		creationData.Id = newId;
		creationData.Materials ;
		creationData.Mesh = mesh;
		u32 numSubs = NumCast<u32>(mesh->components.size());
		//u64 currTotalSubs = data.Materials.size();
		//data.Materials.reserve(currTotalSubs + numSubs);
		for (u32 i = 0; i < numSubs; ++i)
		{
			creationData.Materials.push_back(smc->GetMaterial(i));
		}
		//data.SubPrimitives.push_back({NumCast<u32>(currTotalSubs), numSubs});
		data.Selected.push_back(false);

		return newId;
	}
	else
	{
		ZE_ASSERT(false);
		return InvalidPrimId();
	}
}

void SceneDataInterface::RemovePrimitive(PrimitiveId id)
{
	// TODO
}

void SceneDataInterface::CopyFrameData(u32 from, u32 to)
{
	mData[to].Transforms = mData[from].Transforms;
	//mData[to].Meshes = mData[from].Meshes;
	//mData[to].SubPrimitives = mData[from].SubPrimitives;
	//mData[to].Materials = mData[from].Materials;
	mData[to].RemovedPrimitives.clear();
	mData[to].AddedPrimitives.clear();
	mData[to].Selected = mData[from].Selected;
	u32 const clearSize = NumCast<u32>(min(mData[to].TransformsDirty.size(), mData[from].TransformsDirty.size()));
	mData[to].TransformsDirty.resize(mData[to].Transforms.size(), false);
	for (u32 i = 0; i < clearSize; ++i)
	{
		mData[to].TransformsDirty[i] = false;
	}
}
