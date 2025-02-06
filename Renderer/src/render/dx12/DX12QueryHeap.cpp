#include "DX12QueryHeap.h"
#include "DX12RHI.h"

namespace rnd::dx12
{

DX12QueryHeap::DX12QueryHeap(D3D12_QUERY_HEAP_TYPE type, u32 size)
	:mType(type), mSize(size)
{
	D3D12_QUERY_HEAP_DESC desc;
	desc.Count = size;
	desc.Type = type;
	DXCALL(GetD3D12Device()->CreateQueryHeap(&desc, IID_PPV_ARGS(&mHeap)));
}

DX12QueryHeap::Query DX12QueryHeap::GetQuery()
{
	u32 idx = mCounter++;
	if (idx >= mSize)
	{
		Resize(NumCast<u32>(mSize * 1.5f));
	}
	return {mHeap.Get(), idx};

}

void DX12QueryHeap::Resize(u32 newSize)
{
	GetRHI().DeferredRelease(std::move(mHeap));
	
}

void DX12QueryHeap::OnEndFrame(u64 frameNum, u32 frameIdx)
{
	mCounter = 0;
}

}
