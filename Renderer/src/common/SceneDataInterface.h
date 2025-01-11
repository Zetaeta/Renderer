#pragma once

#include "core/CoreTypes.h"
#include "container/Vector.h"
#include "core/Transform.h"
#include <semaphore>
#include <mutex>
#include "Material.h"

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
class SceneDataInterface
{
public:
	SceneDataInterface();
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
	};

	struct SceneData
	{
		Vector<CompoundMesh*> Meshes;
		Vector<WorldTransform> Transforms;
//		Vector<MaterialID> Materials;
//		Vector<SubPrimitiveRange> SubPrimitives;
		BitVector TransformsDirty;
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

	void FlipBuffer_MainThread()
	{
		WaitForRenderThread();

		ReleaseFrame_MainThread();
		u32 currFrame = mMainThreadIdx;
		mMainThreadIdx = (mMainThreadIdx + 1) % NumSceneDataBuffers;
		StartFrame_MainThread();
		CopyFrameData(currFrame, mMainThreadIdx);
	}

	void BeginFrame_RenderThread()
	{
		mFrameGuards[mRenderThreadIdx].acquire();
	}
	void EndFrame_RenderThread()
	{
		mFrameGuards[mRenderThreadIdx].release();
		mRenderThreadIdx = (mRenderThreadIdx + 1) % NumSceneDataBuffers;
	}

	SceneData& GetMainThreadData()
	{
		return mData[mMainThreadIdx];
	}

	SceneData const& GetMainThreadData() const
	{
		return mData[mMainThreadIdx];
	}

	SceneData const& GetRenderThreadData() const
	{
		return mData[mRenderThreadIdx];
	}
private:

	void WaitForRenderThread()
	{
		u32 nextBuffer = (mMainThreadIdx + 1) % NumSceneDataBuffers;
		mFrameGuards[nextBuffer].acquire();
	}


	void StartFrame_MainThread()
	{
		mFrameGuards[mMainThreadIdx].acquire();
	}
	void ReleaseFrame_MainThread()
	{
		mFrameGuards[mMainThreadIdx].release();
	}

	void CopyFrameData(u32 from, u32 to);


	SceneData mData[NumSceneDataBuffers];

	std::array<std::binary_semaphore, NumSceneDataBuffers> mFrameGuards;

	u32 mMainThreadIdx = 0;
	u32 mRenderThreadIdx = 1;
//	BitVector 
};