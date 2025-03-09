#pragma once
#include "platform/windows/WinEvent.h"
#include "container/MovableObjectPool.h"
#include "SharedD3D12.h"
#include "render/GPUSyncPoint.h"

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

// Currently being passed by value internally, using RefPtr for GPUSyncPoint API.
// TODO Decide actual architecture
class DX12SyncPoint : public GPUSyncPoint, private DX12SyncData
{
public:
	DX12SyncPoint(DX12SyncData const& data, u32 handle)
	:DX12SyncData(data), mHandle(handle) { }

	ZE_COPY_PROTECT(DX12SyncPoint);
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

	void OnFullyReleased() override
	{
		delete this;
	}

	void Release();

	bool Wait(u32 forMS = INFINITE) override
	{
		return WaitInternal(forMS, false);
	}
	bool WaitAndRelease(u32 forMS = INFINITE)
	{
		return WaitInternal(forMS, true);
	}

private:
	bool WaitInternal(u32 forMS, bool thenRelease)
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

