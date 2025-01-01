#include "DX12Upload.h"
#include "DX12RHI.h"
#include <d3dx12.h>

namespace rnd::dx12
{

void DX12UploadContext::FinishUploadSynchronous()
{
	DX12CommandQueue& copyQueue = GetRHI().CmdQueues().Copy;
//	auto lock = copyQueue.Lock();
	DX12SyncPoint syncPt = DX12SyncPointPool::Get()->Claim();
	DXCALL(CmdList->Close());
	copyQueue.LockExecuteSync(CmdList.Get(), syncPt);
	syncPt.Wait();
	
//	copyQueue->ExecuteCommandLists(1, CommandListCast(CmdList.GetAddressOf()));
//	std::lock_guard lock(copyQueue);
}

DX12Uploader::DX12Uploader(ID3D12Device_* device)
{
	for (int i = 0; i < NumContexts; ++i)
	{
		mContexts[i].mOwner = this;
		DXCALL(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mContexts[i].CmdList)));
	}
	DXCALL(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mAllocator)));
}

DX12UploadContext& DX12Uploader::StartUpload()
{
	mAllocator->Reset();
	DX12UploadContext& ctx = mContexts[0];
	DXCALL(ctx.CmdList->Reset(mAllocator.Get(), nullptr));
	return ctx;
}

}
