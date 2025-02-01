#include "ThreadUtils.h"

#include <thread>
#include "container/EnumArray.h"
#include "core/Utils.h"

EnumArray<ENamedThread, std::thread::id> gNamedThreadIds;

bool IsInMainThread()
{
	return true;
	return std::this_thread::get_id() == gNamedThreadIds[ENamedThread::MainThread];
}

bool IsInRenderThread()
{
	return true;
	return std::this_thread::get_id() == gNamedThreadIds[ENamedThread::RenderThread];
}

void Thread::SetNamedThread(ENamedThread thisThread)
{
	ZE_ENSURE(gNamedThreadIds[thisThread] == std::thread::id{});
	gNamedThreadIds[thisThread] = std::this_thread::get_id();
}
