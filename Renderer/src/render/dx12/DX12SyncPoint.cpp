#include "DX12SyncPoint.h"
#include "SharedD3D12.h"

namespace rnd
{
namespace dx12
{

DX12SyncPoint DX12SyncPointPool::Claim()
{
	DX12SyncData* data;
	u32 handle;
	if (mPool.Claim(handle, data))
	{
		 HR_ERR_CHECK(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&data->Fence)));
	}
	data->Value++;
	return DX12SyncPoint(*data, handle);
}

void DX12SyncPointPool::Release(DX12SyncPoint const& sp)
{
	mPool.Release(sp.mHandle);
}

static DX12SyncPointPool sSyncPtPool;

DX12SyncPointPool* DX12SyncPointPool::Get()
{
	return &sSyncPtPool;
}

void DX12SyncPointPool::Create(ID3D12Device_* device)
{
	sSyncPtPool = DX12SyncPointPool(device);
}

void DX12SyncPointPool::Destroy()
{
	sSyncPtPool = {};
}

void DX12SyncPoint::Release() const
{
	DX12SyncPointPool::Get()->Release(*this);
}

}
}

