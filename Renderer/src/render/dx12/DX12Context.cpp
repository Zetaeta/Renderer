#include "DX12Context.h"
#include "DX12RHI.h"
#include <d3dx12.h>
#include "DX12DescriptorTableAllocator.h"
#include "DX12Texture.h"
#include "render/dxcommon/DXGIUtils.h"

namespace rnd::dx12
{

void DX12Context::SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx /*= 0*/)
{
	mGraphicsState.ShaderStates[shader].SRVs.clear();
	mGraphicsState.ShaderStates[shader].SRVs.insert(mGraphicsState.ShaderStates[shader].SRVs.end(), srvs.begin(), srvs.end());
	mGraphicsState.ShaderStates[shader].DescTablesDirty = true;
	//auto& rhi = GetRHI();
	//auto loc = rhi.GetResourceDescTableAlloc()->Reserve(NumCast<u32>(srvs.size()));
	//CD3DX12_CPU_DESCRIPTOR_HANDLE dest(loc.CPUHandle);
	//for (u32 i = 0; i < srvs.size(); ++i)
	//{
	//	auto src = srvs[i].Resource->GetShaderResource<D3D12_CPU_DESCRIPTOR_HANDLE>();
	//	rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
	//}
	//mCmdList->SetDescriptorHeaps(1, &loc.Heap);
	//s8 tablePos = mGraphicsState.RootSig.SRVTableIndices[shader];
	//if (tablePos >= 0)
	//{

	//	mCmdList->SetGraphicsRootDescriptorTable(tablePos, loc.GPUHandle);
	//}
	//else
	//{
	//	ZE_ASSERT(false);
	//}
}

void DX12Context::FinalizeGraphicsState()
{
	bool bResourcesDirty = false;
	u32 requiredResourceDescriptors = 0;
	for (auto stage = mGraphicsState.BeginActiveStages(); stage; ++stage)
	{
		bResourcesDirty |= stage->DescTablesDirty;
		requiredResourceDescriptors += stage->GetRequiredDescriptors();

		for (u32 i=0; i<stage->RootCBVs.size(); ++i)
		{
			mCmdList->SetGraphicsRootConstantBufferView(mGraphicsState.RootSig().RootCBVIndices[stage.Shader], stage->RootCBVs[i]);
		}
	}

	auto& rhi = GetRHI();
	if (mGraphicsState.PSODirty)
	{
		mCmdList->SetPipelineState(rhi.GetPSO(mGraphicsState.PSOState));
		mGraphicsState.PSODirty = false;
	}

	if (!bResourcesDirty)
	{
		return;
	}

	auto loc = rhi.GetResourceDescTableAlloc()->Reserve(NumCast<u32>(requiredResourceDescriptors));
	mCmdList->SetDescriptorHeaps(1, &loc.Heap);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(loc.CPUHandle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE tableStart(loc.GPUHandle);
	for (auto stage = mGraphicsState.BeginActiveStages(); stage; ++stage)
	{
		if (stage->GetRequiredDescriptors() > 0)
		{
			for (UnorderedAccessView& view : stage->UAVs)
			{
				auto src = view.Resource->GetUAV<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId);
				rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
			}
			for (ResourceView& view : stage->SRVs)
			{
				auto src = view.Resource->GetShaderResource<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId);
				rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
			}
			s8 tablePos = mGraphicsState.RootSig().SRVTableIndices[stage.Shader];
			if (tablePos >= 0)
			{
				mCmdList->SetGraphicsRootDescriptorTable(tablePos, tableStart);
			}
			else
			{
				ZE_ASSERT(false);
			}
			tableStart.Offset(NumCast<u32>(stage->SRVs.size()), DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
		}
	}
}

void DX12Context::SetRTAndDS(IRenderTarget::Ref rts, IDepthStencil::Ref ds, int RTArrayIdx, int DSArrayIdx)
{
	SetRTAndDS(Single(rts), ds);
}

void DX12Context::SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds)
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> rtHandles;
	D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = {};
	if (ds)
	{
		dsHandle = ds->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>();
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.DSVFormat, GetDxgiFormat(ds->Desc.Format));
	}
	for (u32 i = 0; i < rts.size(); ++i)
	{
		if (DX12RenderTarget* rt = static_cast<DX12RenderTarget*>(rts[i].get()))
		{
			rtHandles[i] = rt->DescriptorHdl;
			mGraphicsState.UpdatePSO(mGraphicsState.PSOState.RTVFormats[i], GetDxgiFormat(rt->Desc.Format));
		}
	}

	mCmdList->OMSetRenderTargets(NumCast<u32>(rts.size()), &rtHandles[0], false, &dsHandle);
}

void DX12Context::SetConstantBuffers(EShaderType shaderType, Span<IConstantBuffer* const> buffers)
{
	auto& targetVec = mGraphicsState.ShaderStates[shaderType].RootCBVs;
	targetVec.clear();
	for (IConstantBuffer* cb : buffers)
	{
		targetVec.push_back(static_cast<DX12DynamicCB*>(cb)->Commit());
	}
}

void DX12Context::SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const> cbs)
{
	auto& targetVec = mGraphicsState.ShaderStates[shaderType].RootCBVs;
	targetVec.clear();
	for (CBHandle const& cb : cbs)
	{
		targetVec.push_back(cb.UserData.As<D3D12_GPU_VIRTUAL_ADDRESS>());
	}

}

void DX12Context::SetGraphicsShader(EShaderType shaderType, Shader const* shader)
{

	auto& current = mGraphicsState.Shaders()[shaderType];
	if (current != shader)
	{
		current = shader;
		mGraphicsState.PSODirty = true;
	}
	mGraphicsState.SetActive(shaderType);
}

void DX12Context::SetViewport(float width, float height, float TopLeftX, float TopLeftY)
{
	D3D12_VIEWPORT viewport = {
		.TopLeftX = TopLeftX,
		.TopLeftY = TopLeftY,
		.Width = float(width),
		.Height = float(height),
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	mCmdList->RSSetViewports(1, &viewport);

	CD3DX12_RECT scissor(0, 0, NumCast<u32>(width), NumCast<u32>(height));
	mCmdList->RSSetScissorRects(1, &scissor);
}

}
