#include "Logging.h"
#include <thread>
#include "chrono"
#include "iostream"

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

	template<typename Func>
	void ConsumeAll(Func&& func, bool ordered = true)
	{
		Node* head = mHead.exchange(nullptr, std::memory_order_acquire);
		if (head == nullptr)
		{
			return;
		}
		// Reverse chain
		//Node* prev = Head, current = prev->Next, next = nullptr;
		//prev->Next = nullptr;
		//while (current)
		//{
		//
		//}
		//for (Node* current = head, next = head->Next; next != nullptr; current = next, next = next->Next)
		//{
		//	
		//}
		for (Node* current = head; current; current = current->Next)
		{
			func(current->Msg);
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
	LogCategory const* Category = nullptr;
	ELogVerbosity Verbosity;
	String Message;
	ThreadId Thread;
	std::chrono::time_point<std::chrono::system_clock> Time;
};

LogCategory const LogGlobal{ "LogGlobal" };

namespace LogPrivate
{

thread_local char sBuffer[LogBufferSize];
static MPSCMessageQueue<LogMessage> sLogQueue;

void LogImpl(LogCategory const& category, ELogVerbosity verbosity, String&& message)
{
	sLogQueue.Add(LogMessage{ &category, verbosity, std::move(message), CurrentThread(), std::chrono::system_clock::now() });
}

} // namespace LogPrivate

const char* Verbosities[] = {
	"Critical",
	"Error",
	"Warning",
	"Info",
	"Verbose"
};

void LogConsumerThread::DoWork()
{
	LogPrivate::sLogQueue.Wait();
	LogPrivate::sLogQueue.ConsumeAll([](LogMessage const& msg)
	{
//		const std::time_t time = std::chrono::system_clock::to_time_t(msg.Time);
		std::cout << std::format("[{:%F %T}] {}: [{}] {}\n", std::chrono::floor<std::chrono::milliseconds>(msg.Time),
			msg.Category->GetName(), Verbosities[Denum(msg.Verbosity)], msg.Message);
	});
}
