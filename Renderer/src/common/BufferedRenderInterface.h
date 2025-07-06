#pragma once
#include "core/CoreTypes.h"
#include <semaphore>
#include <mutex>
#include "thread/RWLock.h"
#include <unordered_set>
#include "core/Types.h"
#include <array>
#include "container/Vector.h"

template<bool Enabled>
struct ConditionalRWMutex
{
	bool ScopedReadLock() { return true; }
	bool ScopedWriteLock() { return true; }
};

template<>
struct ConditionalRWMutex<true> : public RWLock
{
};

enum class ESelfRegisteringAmount
{
	Few,
	Many
};

template<typename T, ESelfRegisteringAmount Amount>
struct SelfRegisteringContainer
{
	using Type = Vector<T>;
};

template<typename T>
struct SelfRegisteringContainer<T, ESelfRegisteringAmount::Many>
{
	using Type = std::unordered_set<T>;
};


template<typename ActualType, ESelfRegisteringAmount ExpectedAmount, bool IsThreadSafe = true>
class SelfRegistering
{
public:
	SelfRegistering()
	{
		auto lock = sMutex.ScopedWriteLock();
		sInstances.push_back(static_cast<ActualType*>(this));
	}
	~SelfRegistering()
	{
		auto lock = sMutex.ScopedWriteLock();
		RemoveSwap(sInstances, static_cast<ActualType*>(this));
	}

	template<typename Func>
	static void ForEachInstance(Func&& func)
	{
		for (ActualType* instance : sInstances)
		{
			func(instance);
		}
	}

	static SelfRegisteringContainer<ActualType*, ExpectedAmount>::Type sInstances;
	static ConditionalRWMutex<IsThreadSafe> sMutex;
};

template<typename ActualType, ESelfRegisteringAmount ExpectedAmount, bool IsThreadSafe>
SelfRegisteringContainer<ActualType*, ExpectedAmount>::Type SelfRegistering<ActualType, ExpectedAmount, IsThreadSafe>::sInstances;

template<typename ActualType, ESelfRegisteringAmount ExpectedAmount, bool IsThreadSafe>
ConditionalRWMutex<IsThreadSafe> SelfRegistering<ActualType, ExpectedAmount, IsThreadSafe>::sMutex;


class BufferedRenderInterface : public SelfRegistering<BufferedRenderInterface, ESelfRegisteringAmount::Few>
{
public:
	virtual void FlipFrameBuffers(u32 fromIndex, u32 toIndex) = 0;

	static u32 sMainThreadIdx;
	static u32 sRenderThreadIdx;
	constexpr static u32 NumSceneDataBuffers = 2;

	static void WaitForRenderThread()
	{
		u32 nextBuffer = (sMainThreadIdx + 1) % NumSceneDataBuffers;
		sFrameGuards[nextBuffer].acquire();
	}

	static void FlipBuffer_MainThread()
	{
		WaitForRenderThread();

		ReleaseFrame_MainThread();
		u32 currFrame = sMainThreadIdx;
		sMainThreadIdx = (sMainThreadIdx + 1) % NumSceneDataBuffers;
		ForEachInstance([=](BufferedRenderInterface* instance)
		{
			instance->FlipFrameBuffers(currFrame, sMainThreadIdx);
		});
		StartFrame_MainThread();
	}

	static void StartFrame_MainThread()
	{
//		mFrameGuards[mMainThreadIdx].acquire();
	}
	static void ReleaseFrame_MainThread()
	{
		sFrameGuards[sMainThreadIdx].release();
	}

	static void BeginFrame_RenderThread()
	{
		sFrameGuards[sRenderThreadIdx].acquire();
	}
	static bool TryBeginFrame_RenderThread()
	{
		return sFrameGuards[sRenderThreadIdx].try_acquire();
	}

	static void EndFrame_RenderThread()
	{
		sFrameGuards[sRenderThreadIdx].release();
		sRenderThreadIdx = (sRenderThreadIdx + 1) % NumSceneDataBuffers;
	}
	static std::array<std::binary_semaphore, NumSceneDataBuffers> sFrameGuards;
};

template<typename T>
class TBufferedRenderInterface : public BufferedRenderInterface
{
public:
	T& GetData_MainThread()
	{
		return mData[sMainThreadIdx];
	}
	T const& GetData_MainThread() const
	{
		return mData[sMainThreadIdx];
	}

	T const& GetData_RenderThread() const
	{
		return mData[sRenderThreadIdx];
	}

protected:
	std::array<T, NumSceneDataBuffers> mData;

};
