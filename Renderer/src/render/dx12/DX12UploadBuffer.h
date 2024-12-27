#pragma once

#include "SharedD3D12.h"

class DX12UploadBuffer
{
	DX12UploadBuffer(size_t startSize);
	ComPtr<ID3D12Resource> mUploadHeap;
};

