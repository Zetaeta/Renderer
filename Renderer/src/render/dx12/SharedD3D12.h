#pragma once

#include <d3d12.h>
#include "core/Maths.h"
#include "core/WinUtils.h"

#define DXCALL(expr) HR_ERR_CHECK(expr)

using ID3D12Device_ = ID3D12Device6;
using ID3D12GraphicsCommandList_ = ID3D12GraphicsCommandList4;
using ID3D12Resource_ = ID3D12Resource;

namespace rnd::dx12
{
class DX12RHI;
class DX12Allocator;

}

enum ESwapchainBufferCount : u8
{
	DoubleBuffered = 2,
	TripleBuffered = 3,
	MaxSwapchainBufferCount = TripleBuffered
};

