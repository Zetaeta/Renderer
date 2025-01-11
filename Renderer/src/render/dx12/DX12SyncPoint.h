#pragma once
#include "platform/windows/WinEvent.h"
#include "container/MovableObjectPool.h"
#include "SharedD3D12.h"

namespace rnd
{
namespace dx12
{

struct DX12SyncData
{
	ComPtr<ID3D12Fence> Fence;
	u64 Value = 0;
};

class DX12SyncPointPool;

class DX12SyncPoint : private DX12SyncData
{
public:
	DX12SyncPoint(DX12SyncData const& data, u32 handle)
	:DX12SyncData(data), mHandle(handle) { }

	RCOPY_PROTECT(DX12SyncPoint);
	RMOVE_DEFAULT(DX12SyncPoint);

	~DX12SyncPoint()
	{
		Release();
	}

	void GPUSignal(ID3D12CommandQueue* queue)
	{
		queue->Signal(Fence.Get(), Value);
	}

	void GPUWait(ID3D12CommandQueue* queue)
	{
		queue->Wait(Fence.Get(), Value);
	}

	void CPUSignal()
	{
		Fence->Signal(Value);
	}

	bool IsPassed() const
	{
		return Fence->GetCompletedValue() >= Value;
	}

	wnd::WinEvent GetCompletionEvent() const
	{
		wnd::WinEvent evt = wnd::ClaimEvent(false);
		Fence->SetEventOnCompletion(Value, evt.GetHandle());
		return evt;
	}

	void Release();

	bool Wait(u32 forMS = INFINITE, bool thenRelease = true)
	{
		if (IsPassed())
		{
			return true;
		}

		auto evt = GetCompletionEvent();
		bool result = evt.Wait(forMS);
		wnd::ReleaseEvent(evt);
		if (thenRelease)
		{
			Release();
		}
		return result;
	}
private:
	friend DX12SyncPointPool;
	u32 mHandle = 0;
};

class DX12SyncPointPool
{
public:
	DX12SyncPointPool() = default;
	DX12SyncPointPool(ID3D12Device_* device)
	:mPool(8), mDevice(device)
	{
	}

	DX12SyncPoint Claim();
	void Release(DX12SyncPoint const& sp);

	static DX12SyncPointPool* Get();
	static void Create(ID3D12Device_* device);
	static void Destroy();


private:
	MovableObjectPool<DX12SyncData> mPool;
	ComPtr<ID3D12Device_> mDevice = nullptr;
};

}
}

