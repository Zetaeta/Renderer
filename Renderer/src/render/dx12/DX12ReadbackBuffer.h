#pragma once
#include "DX12UploadBuffer.h"
#include "render/ManualRingBuffer.h"

namespace rnd::dx12
{
class DX12ReadbackBuffer : public TDX12RingBuffer<ManualRingBuffer<8>>
{
public:
	DX12ReadbackBuffer(size_t size)
		: TDX12RingBuffer(size, D3D12_HEAP_TYPE_READBACK)
	{
	}

	void PushFrame()
	{
		ManualRingBuffer::PushFrame();
	}

	void PopFrame()
	{
		ManualRingBuffer::PushFrame();
	}
};

}
