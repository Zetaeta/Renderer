#include "ThreadUtils.h"

#include <thread>
#include "container/EnumArray.h"
#include "core/Utils.h"

EnumArray<std::thread::id, ENamedThread> gNamedThreadIds;

bool IsInMainThread()
{
	return std::this_thread::get_id() == gNamedThreadIds[ENamedThread::MainThread];
}

bool IsInRenderThread()
{
	return std::this_thread::get_id() == gNamedThreadIds[ENamedThread::RenderThread];
}

void Thread::SetNamedThread(ENamedThread thisThread)
{
	ZE_ENSURE(gNamedThreadIds[thisThread] == std::thread::id{});
	gNamedThreadIds[thisThread] = std::this_thread::get_id();
}
