#pragma once
#include <functional>
#include <mutex>
#include <queue>

template<typename T>
class MessageQueue
{
public:
	template<typename TArg>
	void push(TArg&& t)
	{
		{
			std::lock_guard lock {m_Mutex};
			m_Queue.push(t);
		}
		m_Cv.notify_one();
	}

	template<typename TArg>
	void Push(TArg&& t)
	{
		push(std::forward<TArg>(t));
	}

	T pop()
	{
		std::unique_lock lock {m_Mutex};
		m_Cv.wait(lock, [&] { return !m_Queue.empty(); });
		T t = std::move(m_Queue.front());
		m_Queue.pop();
		return t;
	}

	T Pop()
	{
		return pop();
	}

	bool IsEmpty()
	{
		std::lock_guard lock {m_Mutex};
		return m_Queue.empty();
	}
	bool TryPop(T& outVal)
	{
		std::lock_guard lock {m_Mutex};
		if (m_Queue.empty())
		{
			return false;
		}
		else
		{
			outVal = std::move(m_Queue.front());
			m_Queue.pop();
			return true;
		}
	}
		
private:
	std::mutex m_Mutex;
	std::condition_variable m_Cv;
	std::queue<T> m_Queue;
};

template<typename FuncType>
class CommandPipe : public MessageQueue<std::function<FuncType>>
{
public:
	using CmdType = std::function<FuncType>;
};
