#include "Logging.h"
#include <thread>
#include "chrono"
#include "iostream"
#include <semaphore>
#include "thread/MPSCMessageQueue.h"
#include "container/EnumArray.h"

using ThreadId = std::thread::id;

ThreadId CurrentThread()
{
	return std::this_thread::get_id();
}

struct LogMessage
{
	using MsgTime = std::chrono::time_point<std::chrono::system_clock>;
	LogMessage(LogCategory const& cat, ELogVerbosity verbosity, String&& msg, ThreadId thread, MsgTime time)
		:Category(&cat), Message(std::move(msg)), Thread(thread), Verbosity(verbosity), Time(time)
	{
	}

	LogMessage(std::binary_semaphore& sema)
	:FlushSemaphore(&sema), Verbosity(ELogVerbosity::Flush)
	{
		sema.acquire();
	}
	LogMessage(std::nullptr_t)
	:FlushSemaphore(nullptr), Verbosity(ELogVerbosity::Flush)
	{
	}

	union 
	{
		LogCategory const* Category = nullptr;
		std::binary_semaphore* FlushSemaphore;
	};
	String Message;
	ThreadId Thread;
	ELogVerbosity Verbosity;
	MsgTime Time;
};

LogCategory const LogGlobal{ "LogGlobal" };

namespace LogPrivate
{

thread_local char sBuffer[LogBufferSize];
static MPSCMessageQueue<LogMessage> sLogQueue;
static LogConsumerThread* sLogThread = nullptr;

void LogImpl(LogCategory const& category, ELogVerbosity verbosity, String&& message)
{
	sLogQueue.Add(LogMessage{ category, verbosity, std::move(message), CurrentThread(), std::chrono::system_clock::now() });
}

} // namespace LogPrivate

const char* Verbosities[] = {
	"Critical",
	"Error",
	"Warning",
	"Info",
	"Verbose"
};

void FlushLog()
{
	LogPrivate::sLogThread->Flush();
}

LogConsumerThread::LogConsumerThread()
{
	LogPrivate::sLogThread = this;
}

void LogConsumerThread::Flush()
{
	std::binary_semaphore sema(1);
	LogPrivate::sLogQueue.Add(sema);
	sema.acquire();
}

static EnumArray<ELogVerbosity, const char*> sColourCodes =
{
	"\x1b[1",
	"\x1b[1",
	"\x1b[3",
	"",
	""
};

void LogConsumerThread::DoWork()
{
	if (LogPrivate::sLogQueue.IsEmpty())
	{
		LogPrivate::sLogQueue.Wait();
	}
	else
	{
		LogPrivate::sLogQueue.ConsumeAll([](LogMessage const& msg)
		{
			if (msg.Verbosity == ELogVerbosity::Flush)
			{
				if (msg.FlushSemaphore)
				{
					msg.FlushSemaphore->release();
				}
			}
			else
			{
				
				std::cout << std::format("{}[{:%F %T}] {}: [{}] {}\n", sColourCodes[msg.Verbosity], std::chrono::floor<std::chrono::milliseconds>(msg.Time),
					msg.Category->GetName(), Verbosities[Denum(msg.Verbosity)], msg.Message);
			}
	//		const std::time_t time = std::chrono::system_clock::to_time_t(msg.Time);
		});
	}
}

void LogConsumerThread::RequestStop()
{
	WorkerThread::RequestStop();
	LogPrivate::sLogQueue.Add(nullptr);
}
