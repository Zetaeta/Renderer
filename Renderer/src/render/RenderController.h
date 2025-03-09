#pragma once
#include "RndFwd.h"
#include "container/Vector.h"
#include "RenderingThread.h"

namespace rnd
{
// Serves as an interface to potentially multiple render threads
class RenderController
{
public:
	void BeginRenderFrame();
	void EndRenderFrame();

	void AddRenderBackend(IRenderDevice* rhi);
	void RemoveRenderBackend(IRenderDevice* rhi);

	void EnqueueRenderCommand(std::function<RenderCmdFunc>&& command, IRenderDevice* backend, const char* debugName);
	void FlushRenderCommands(IRenderDevice* backend);

	void RequestExit()
	{
		mExitRequested.store(true, std::memory_order_release);
	}
	bool IsExitRequested()
	{
		return mExitRequested.load(std::memory_order_acquire);
	}

private:
	std::atomic<bool> mExitRequested = false;
//	Vector<IRenderDevice*> mBackends;
	Vector<OwningPtr<RenderingThread>> mRenderThreads;
//	SmallVector<OwningPtr<CommandPipe<RenderCmdFunc>>, 2> mPipes;
//	OwningPtr<CommandPipe<RenderCmdFunc>> mGlobalPipe;
};

extern RenderController GRenderController;
}

template<typename CommandType>
inline void RenderCommand(CommandType&& cmd, rnd::IRenderDevice* forBackend = nullptr, const char* debugName = nullptr)
{
	rnd::GRenderController.EnqueueRenderCommand(cmd, forBackend, debugName);
}

inline void FlushRenderCommands(rnd::IRenderDevice* forBackend = nullptr)
{
	rnd::GRenderController.FlushRenderCommands(forBackend);
}
