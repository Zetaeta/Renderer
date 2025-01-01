#include "DX12Allocator.h"

#include "DX12RHI.h"

#include <d3dx12.h>

namespace rnd::dx12
{

DX12ResourceVal DX12Allocator::Allocate(D3D12_RESOURCE_DESC const& inDesc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE const* clearVal)
{
	auto desc = inDesc;
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	}
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ComPtr<ID3D12Resource> resource;
	DXCALL(mDevice->CreateCommittedResource(&heapProps, mFlags, &desc, initialState, clearVal, IID_PPV_ARGS(&resource)));
	return resource;
}

DX12Allocator::DX12Allocator(ID3D12Device_* device)
:mDevice(device)
{
}

void DX12Allocator::Free(DX12ResourceVal resource)
{
	GetRHI().DeferredRelease(resource);
}

}
