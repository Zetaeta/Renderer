#pragma once

#include "SharedD3D12.h"

namespace rnd::dx12
{

using DX12ResourceVal = ComPtr<ID3D12Resource>;

class DX12Allocator
{
public:
	DX12Allocator(ID3D12Device_* device, D3D12_HEAP_TYPE type = D3D12_HEAP_TYPE_DEFAULT);
	virtual DX12ResourceVal Allocate(D3D12_RESOURCE_DESC const& desc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE const* clearVal);
	virtual void Free(DX12ResourceVal resource);

private:	
	D3D12_HEAP_FLAGS mFlags = D3D12_HEAP_FLAG_NONE;
	D3D12_HEAP_TYPE mHeapType = D3D12_HEAP_TYPE_DEFAULT;
	ID3D12Device_* mDevice = nullptr;
 };

}
