#include "Logging.h"
#include <thread>
#include "chrono"
#include "iostream"
#include <semaphore>

template<typename Message>
class MPSCMessageQueue
{
	RCOPY_MOVE_PROTECT(MPSCMessageQueue);
public:
	MPSCMessageQueue() = default;
	template<typename TMessage>
	void Add(TMessage&& msg)
	{
		Node* node = new Node { mHead.load(std::memory_order_acquire), std::forward<TMessage>(msg) };
		while (!mHead.compare_exchange_weak(node->Next, node)) { }
		mHead.notify_one();
	}

	void Wake()
	{
		mHead.notify_one();
	}

	template<typename Func>
	void ConsumeAll(Func&& func, bool ordered = true)
	{
		Node* head = mHead.exchange(nullptr, std::memory_order_acquire);
		if (head == nullptr)
		{
			return;
		}
		// Reverse chain
		Node* prev = head;
		Node* current = prev->Next;
		Node* next = nullptr;
		prev->Next = nullptr;
		while (current)
		{
			next = current->Next;
			current->Next = prev;
			prev = current;
			current = next;
		}
		for (Node* n = prev; n; n = n->Next)
		{
			func(n->Msg);
		}
	}

	bool IsEmpty()
	{
		return mHead.load(std::memory_order_acquire) == nullptr;
	}

	void Wait()
	{
		mHead.wait(nullptr);
	}

private:
	struct Node
	{
		Node* Next = nullptr;
		Message Msg;
	};

	std::atomic<Node*> mHead = nullptr;
};

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
				std::cout << std::format("[{:%F %T}] {}: [{}] {}\n", std::chrono::floor<std::chrono::milliseconds>(msg.Time),
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
