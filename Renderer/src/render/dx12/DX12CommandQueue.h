#pragma once

#include "SharedD3D12.h"
#include <mutex>

namespace rnd
{
namespace dx12
{
class DX12CommandQueue
{
public:
	ZE_COPY_PROTECT(DX12CommandQueue);

	DX12CommandQueue() = default;

	DX12CommandQueue(ID3D12Device_* device, D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE)
	{
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type = type;
		desc.Flags = flags;
		device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mQueue));
	}

	DX12CommandQueue& operator=(DX12CommandQueue&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}
		std::lock_guard mylock(mMutex);
		std::lock_guard olock(other.mMutex);
		mQueue = std::move(other.mQueue);
		return *this;
	}

	template<typename LockType = std::lock_guard<std::mutex>>
	[[nodiscard]] LockType Lock()
	{
		return LockType(mMutex);
	}


	ID3D12CommandQueue* operator->() const
	{
		return mQueue.Get();
	}

	operator ID3D12CommandQueue* ()
	{
		return mQueue.Get();
	}

	ID3D12CommandQueue* Get() const
	{
		return mQueue.Get();
	}

	operator std::mutex&()
	{
		return mMutex;
	}

	void LockExecuteSync(ID3D12CommandList* cmdList, DX12SyncPoint& syncPoint)
	{
		auto lock = Lock();
		mQueue->ExecuteCommandLists(1, &cmdList);
		syncPoint.GPUSignal(Get());
	}


private:
	ComPtr<ID3D12CommandQueue> mQueue;
	std::mutex mMutex;
};


struct DX12CommandQueues 
{
	DX12CommandQueue Direct;
	DX12CommandQueue Copy;
	DX12CommandQueue Compute;
};

}
}

