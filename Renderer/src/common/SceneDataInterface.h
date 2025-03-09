#pragma once

#include "core/CoreTypes.h"
#include "container/Vector.h"
#include "core/Transform.h"
#include <semaphore>
#include <mutex>
#include "Material.h"
#include "BufferedRenderInterface.h"

class StaticMeshComponent;
struct CompoundMesh;
using PrimitiveComponent = StaticMeshComponent;

using PrimitiveId = u32;

constexpr PrimitiveId InvalidPrimId()
{
	return (PrimitiveId) -1;
}

/**
 * 
 */
class SceneDataInterface : public BufferedRenderInterface
{
public:
	SceneDataInterface();
	~SceneDataInterface();
	SceneDataInterface& operator=(SceneDataInterface&& other) = default;
	SceneDataInterface(SceneDataInterface&& other) = default;

//	static Span<SceneDataInterface*> GetAll();
	/**
	 * 1 = rendering on main thread.
	 * Assuming separate render thread(s), 2 buffers means the main and render thread have to synchronize
	 * at a single point to pass scene data from main to render thread.
	 * 3 allows the render thread to fall behind by up to one frame.
	 * The main thread will write to one set of buffers at a time, all others are read-only.
	 */
	constexpr static u32 NumSceneDataBuffers = 2;

	struct SubPrimitiveRange
	{
		u32 Start = 0;
		u32 Count = 1;
	};

	struct SMCreationData
	{
		std::shared_ptr<CompoundMesh> Mesh;
		SmallVector<MaterialID, 4> Materials;
		PrimitiveId Id;
		ScreenObjectId ScreenId;
	};

	struct SceneData
	{
//		Vector<CompoundMesh*> Meshes;
		Vector<WorldTransform> Transforms;
		//Vector<bool> Selected;
		//Vector<ScreenObjectId> ScreenObjIds;
//		Vector<MaterialID> Materials;
//		Vector<SubPrimitiveRange> SubPrimitives;
		BitVector TransformsDirty;
		BitVector Selected;
		Vector<SMCreationData> AddedPrimitives;
		Vector<PrimitiveId> RemovedPrimitives;
		SceneData() = default;
		RCOPY_MOVE_PROTECT(SceneData);
	};

	PrimitiveId AddPrimitive(PrimitiveComponent* component);
	void RemovePrimitive(PrimitiveId id);

	void UpdateTransform_MainThread(PrimitiveId id, WorldTransform const& transform)
	{
		auto& data = GetMainThreadData();
		data.Transforms[id] = transform;
		data.TransformsDirty[id] = true;
	}

	void UpdateSelected_MainThread(PrimitiveId id, bool selected)
	{
		auto& data = GetMainThreadData();
		data.Selected[id] = selected;
	}

	//void FlipBuffer_MainThread()
	//{
	//	WaitForRenderThread();

	//	ReleaseFrame_MainThread();
	//	u32 currFrame = mMainThreadIdx;
	//	mMainThreadIdx = (mMainThreadIdx + 1) % NumSceneDataBuffers;
	//	StartFrame_MainThread();
	//	CopyFrameData(currFrame, mMainThreadIdx);
	//}

	//void BeginFrame_RenderThread()
	//{
	//	mFrameGuards[mRenderThreadIdx].acquire();
	//}
	//void EndFrame_RenderThread()
	//{
	//	mFrameGuards[mRenderThreadIdx].release();
	//	mRenderThreadIdx = (mRenderThreadIdx + 1) % NumSceneDataBuffers;
	//}

	SceneData& GetMainThreadData()
	{
		return mData[sMainThreadIdx];
	}

	SceneData const& GetMainThreadData() const
	{
		return mData[sMainThreadIdx];
	}

	SceneData const& GetRenderThreadData() const
	{
		return mData[sRenderThreadIdx];
	}
private:

	//void WaitForRenderThread()
	//{
	//	u32 nextBuffer = (sMainThreadIdx + 1) % NumSceneDataBuffers;
	//	sFrameGuards[nextBuffer].acquire();
	//}


//	void StartFrame_MainThread()
//	{
////		mFrameGuards[mMainThreadIdx].acquire();
//	}
//	void ReleaseFrame_MainThread()
//	{
//		mFrameGuards[mMainThreadIdx].release();
//	}

	void FlipFrameBuffers(u32 from, u32 to) override;


	SceneData mData[NumSceneDataBuffers];

	//std::array<std::binary_semaphore, NumSceneDataBuffers> mFrameGuards;

	//u32 mMainThreadIdx = 0;
	//u32 mRenderThreadIdx = 0;
//	BitVector 
};
