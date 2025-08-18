#include "SceneDataInterface.h"
#include "scene/StaticMeshComponent.h"

//static Vector<SceneDataInterface*> sInterfaces;
std::mutex sInterfacesMtx;

SceneDataInterface::SceneDataInterface()
{
//	sInterfaces.push_back(this);
}

 SceneDataInterface::~SceneDataInterface()
{
//	Remove(sInterfaces, this);
}

PrimitiveId SceneDataInterface::SceneData::AllocatePrimitiveSlot()
{
	PrimitiveId newId = NumCast<PrimitiveId>(Transforms.size());
	Transforms.emplace_back();
	TransformsDirty.push_back(true);
	Selected.push_back(false);
	return newId;
}

PrimitiveId SceneDataInterface::AddPrimitive(PrimitiveComponent* component)
{
	;
	auto& data = GetMainThreadData();
	PrimitiveId newId = data.AllocatePrimitiveSlot();
	data.Transforms[newId] = component->GetWorldTransform();
	if (StaticMeshComponent* smc = Cast<StaticMeshComponent>(component))
	{
		SMCreationData& creationData = data.AddedStaticMeshes.emplace_back();
		creationData.ScreenId = component->GetScreenId();
		creationData.Id = newId;
		creationData.Mesh = smc->GetMeshRef();
		u32 numSubs = NumCast<u32>(creationData.Mesh->components.size());
		for (u32 i = 0; i < numSubs; ++i)
		{
			creationData.Materials.push_back(smc->GetMaterial(i));
		}

		return newId;
	}
	else 
	{
//		if (auto Drawable = component->CreateDrawable())
		auto& creationData = data.AddedCustomDrawables.emplace_back();
		creationData.ScreenId = component->GetScreenId();
		rnd::ForEachDevice([&](rnd::IRenderDevice* device, u32 i)
		{
			creationData.Drawable[i] = component->CreateDrawable();
		});
		creationData.Id = newId;
		return newId;
	}
}

void SceneDataInterface::RemovePrimitive(PrimitiveId id)
{
	// TODO
}

void SceneDataInterface::FlipFrameBuffers(u32 from, u32 to)
{
	mData[to].Transforms = mData[from].Transforms;
	//mData[to].Meshes = mData[from].Meshes;
	//mData[to].SubPrimitives = mData[from].SubPrimitives;
	//mData[to].Materials = mData[from].Materials;
	mData[to].RemovedPrimitives.clear();
	mData[to].AddedStaticMeshes.clear();
	mData[to].AddedCustomDrawables.clear();
	mData[to].Selected = mData[from].Selected;
	u32 const clearSize = NumCast<u32>(min(mData[to].TransformsDirty.size(), mData[from].TransformsDirty.size()));
	mData[to].TransformsDirty.resize(mData[to].Transforms.size(), false);
	for (u32 i = 0; i < clearSize; ++i)
	{
		mData[to].TransformsDirty[i] = false;
	}
}
