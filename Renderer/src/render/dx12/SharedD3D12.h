#pragma once

#include <d3d12.h>
#include "core/CoreTypes.h"
#include "core/Maths.h"
#include "core/WinUtils.h"
#include "render/dxcommon/SharedD3D.h"
#include "render/DeviceResource.h"

using ID3D12Device_ = ID3D12Device6;
using ID3D12GraphicsCommandList_ = ID3D12GraphicsCommandList4;
using ID3D12Resource_ = ID3D12Resource;

namespace rnd::dx12
{
using namespace rnd::dx;

class DX12RHI;
class DX12Allocator;
class DX12Texture;
	using DX12TextureRef=std::shared_ptr<DX12Texture>;

constexpr bool IsValid(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	return cpuHandle.ptr != 0;
}

ID3D12Resource_* GetD3D12Resource(IDeviceResource* resource);

}

enum ESwapchainBufferCount : u8
{
	DoubleBuffered = 2,
	TripleBuffered = 3,
	MaxSwapchainBufferCount = TripleBuffered
};


