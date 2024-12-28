#pragma once
#include <thread>
#include <atomic>

class WorkerThread
{
public:
	void Start();
	virtual void RequestStop();
	void Join();


	virtual ~WorkerThread();

	virtual void DoWork() = 0;
	virtual void Main();

protected:
	void Sleep(float ms);

	std::atomic<bool> mStopRequested = false;
	std::thread		  mThread;
};
