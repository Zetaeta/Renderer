#include "RenderingThread.h"
#include "RenderDevice.h"

namespace rnd
{

RenderingThread::RenderingThread(IRenderDevice* device)
	: mDevice(device)
{
	device->MainThread = this;
	mThreadId = std::this_thread::get_id();
}

void RenderingThread::RenderFrame()
{
	mDevice->RenderFrame();
}

void RenderingThread::AddCommand(RenderCommandPipe::CmdType command, const char* name)
{
	bool needsQueue = !IsCurrentThread() ||
		!mDevice->GetImmediateContext([&command](IRenderDeviceCtx& ctx) {
			command(ctx);
		});
	if (needsQueue)
	{
		mPipe.Push(command);
	}
}

void RenderingThread::FlushCommands()
{
	//TODO multithreadin
}

}
