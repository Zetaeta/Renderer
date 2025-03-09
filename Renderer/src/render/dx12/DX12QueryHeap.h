#pragma once
#include "SharedD3D12.h"
#include "DX12EndFrameInterface.h"

namespace rnd::dx12
{
class DX12QueryHeap : public IDX12EndFrameInterface
{
public:
	DX12QueryHeap(D3D12_QUERY_HEAP_TYPE type, u32 size);

	struct Query
	{
		ID3D12QueryHeap* Heap;
		u32 Index;
	};

	Query GetQuery();

	void OnEndFrame(u64 frameNum, u32 frameIdx) override;

	void Resize(u32 newSize);

private:
	D3D12_QUERY_HEAP_TYPE mType;
	u32 mSize;
	u32 mCounter = 0; // TODO figure out how often to reuse queries. Currently single-use each frame. Might need only one per thread/cmdlist?
	ComPtr<ID3D12QueryHeap> mHeap;
//	std::array<DX12ReadbackAllocation, MaxSwapchainBufferCount> mReadbacks; 
};

struct DX12TimingQuery
{
//	ID3D12Resource* ReadbackBuffer = nullptr;
//	u64 Offset = 0;
	u64 const* Location = nullptr;
};

struct DX12Timer : public GPUTimer
{
	struct IntervalData
	{
		DX12TimingQuery Start;
		DX12TimingQuery End;
	};
	std::array<IntervalData, MaxSwapchainBufferCount> PerFrameData;
};

}
