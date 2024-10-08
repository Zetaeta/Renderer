#include "WorkerThread.h"
#include "Utils.h"



void WorkerThread::Start()
{
	mThread = std::thread([this] { Main(); });
}

void WorkerThread::RequestStop()
{
	mStopRequested.store(true, std::memory_order_release);
}

void WorkerThread::Join()
{
	if (RCHECK(mThread.joinable()))
	{
		mThread.join();
	}
}

void WorkerThread::Main()
{
	while (!mStopRequested.load(std::memory_order_acquire))
	{
		DoWork();
	}
}
