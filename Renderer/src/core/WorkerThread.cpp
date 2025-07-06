#include "WorkerThread.h"
#include "Utils.h"
#include "Maths.h"



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
	RequestStop();
	if (RCHECK(mThread.joinable()))
	{
		mThread.join();
	}
}

WorkerThread::~WorkerThread()
{
	if (mThread.joinable())
	{
		Join();
	}
}

void WorkerThread::Main()
{
	while (!mStopRequested.load(std::memory_order_acquire))
	{
		DoWork();
	}
}

void WorkerThread::Sleep(float ms)
{
	RCHECK(std::this_thread::get_id() == mThread.get_id());
	std::this_thread::sleep_for(std::chrono::microseconds(NumCast<s64>(ms * 1000.f)));
}
