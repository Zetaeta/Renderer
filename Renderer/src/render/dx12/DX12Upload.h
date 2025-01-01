#pragma once

#include "SharedD3D12.h"

namespace rnd::dx12
{
struct DX12UploadContext
{
	ComPtr<ID3D12GraphicsCommandList_> CmdList;
	void FinishUploadSynchronous();
private:
	friend class DX12Uploader;
	DX12Uploader* mOwner = nullptr;
};

class DX12Uploader
{
public:
	constexpr static u32 NumContexts = 1;

	RCOPY_PROTECT(DX12Uploader);
	RMOVE_DEFAULT(DX12Uploader);

	DX12Uploader() = default;
	DX12Uploader(ID3D12Device_* device);
	
	DX12UploadContext& StartUpload();

	DX12UploadContext mContexts[NumContexts];
	ComPtr<ID3D12GraphicsCommandList_> CmdList;
	ComPtr<ID3D12CommandAllocator> mAllocator;
};
}
