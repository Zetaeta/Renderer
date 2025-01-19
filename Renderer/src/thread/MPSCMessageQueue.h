#pragma once

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
