#include "DX12Context.h"
#include "DX12RHI.h"
#include <d3dx12.h>
#include "DX12DescriptorTableAllocator.h"

namespace rnd::dx12
{

void DX12Context::SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx /*= 0*/)
{
	auto& rhi = GetRHI();
	auto loc = rhi.GetResourceDescTableAlloc()->Reserve(NumCast<u32>(srvs.size()));
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(loc.CPUHandle);
	for (u32 i = 0; i < srvs.size(); ++i)
	{
		auto src = srvs[i].Resource->GetShaderResource<D3D12_CPU_DESCRIPTOR_HANDLE>();
		rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
	}
	mCmdList->SetDescriptorHeaps(1, &loc.Heap);
	mCmdList->SetGraphicsRootDescriptorTable(1, loc.GPUHandle);
}

}
