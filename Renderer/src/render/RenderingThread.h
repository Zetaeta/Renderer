#pragma once
#include "RndFwd.h"
#include "thread/CommandPipe.h"
namespace rnd
{
using RenderCmdFunc = void(IRenderDeviceCtx& ctx);
using RenderCommandPipe = CommandPipe<RenderCmdFunc>;
class RenderingThread
{
public:
	RenderingThread(IRenderDevice* device);
	RenderCommandPipe& GetPipe()
	{
		return mPipe;
	}

	void RenderFrame();
	void ProcessCommands();

	IRenderDevice* GetDevice() const
	{
		return mDevice;
	}

	void AddCommand(RenderCommandPipe::CmdType command, const char* name);

	bool IsCurrentThread()
	{
		return std::this_thread::get_id() == mThreadId;
	}

	void FlushCommands();

private:
	IRenderDevice* mDevice;
	CommandPipe<RenderCmdFunc> mPipe;
	std::thread::id mThreadId;

};

}
