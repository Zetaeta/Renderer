#pragma once
#include <thread>
#include <atomic>

class WorkerThread
{
public:
	void Start();
	void RequestStop();
	void Join();


	virtual void DoWork() = 0;
	virtual void Main();

	std::atomic<bool> mStopRequested = false;
	std::thread		  mThread;
};
