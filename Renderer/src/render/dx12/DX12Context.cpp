#include "DX12Context.h"
#include "DX12RHI.h"
#include <d3dx12.h>
#include "DX12DescriptorTableAllocator.h"
#include "DX12Texture.h"
#include "render/dxcommon/DXGIUtils.h"
#include "render/RenderContext.h"
#include "render/ShadingCommon.h"

namespace rnd::dx12
{

void DX12Context::SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx /*= 0*/)
{
	if (shader < EShaderType::GraphicsCount)
	{
		mGraphicsState.ShaderStates[shader].SRVs.clear();
		mGraphicsState.ShaderStates[shader].SRVs.insert(mGraphicsState.ShaderStates[shader].SRVs.end(), srvs.begin(), srvs.end());
		mGraphicsState.ShaderStates[shader].DescTablesDirty = true;
	}
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

	auto& rhi = GetRHI();
	if (mGraphicsState.PSODirty)
	{
		mGraphicsState.PSOState.RootSig = DX12GraphicsRootSignature::MakeStandardRootSig(2, 2, 0, 0);
		mGraphicsState.RootSigDirty = true;
		mCmdList->SetPipelineState(rhi.GetPSO(mGraphicsState.PSOState));
		mGraphicsState.PSODirty = false;
	}

	if (mGraphicsState.RootSigDirty)
	{
		mCmdList->SetGraphicsRootSignature(mGraphicsState.RootSig().Get());
	}

	for (auto stage = mGraphicsState.BeginActiveStages(); stage; ++stage)
	{
		bResourcesDirty |= stage->DescTablesDirty;
		requiredResourceDescriptors += stage->GetRequiredDescriptors();

		for (u32 i=0; i<stage->RootCBVs.size(); ++i)
		{
			mCmdList->SetGraphicsRootConstantBufferView(mGraphicsState.RootSig().RootCBVIndices[stage.Shader], stage->RootCBVs[i]);
		}
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
				if (view.Resource)
				{
					auto src = view.Resource->GetShaderResource<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId);
					rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
				}
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
	SetRTAndDS(rts? Single(rts) : Span<IRenderTarget::Ref>{(IRenderTarget::Ref*)nullptr, 0}, ds);
}

void DX12Context::SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds)
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> rtHandles {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = {};
	if (ds)
	{
		dsHandle = ds->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>();
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.DSVFormat, GetDxgiFormat(ds->Desc.Format, ETextureFormatContext::DepthStencil));
	}
	else
	{
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.DSVFormat, DXGI_FORMAT_UNKNOWN);

	}
	for (u32 i = 0; i < rts.size(); ++i)
	{
		if (DX12RenderTarget* rt = static_cast<DX12RenderTarget*>(rts[i].get()))
		{
			rt->BaseTex->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			rtHandles[i] = rt->DescriptorHdl;
			mGraphicsState.UpdatePSO(mGraphicsState.PSOState.RTVFormats[i], GetDxgiFormat(rt->Desc.Format, ETextureFormatContext::RenderTarget));
		}
	}
	for (auto i = rts.size(); i < 8; ++i)
	{
		mGraphicsState.PSOState.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	mGraphicsState.UpdatePSO(mGraphicsState.PSOState.NumRTs, NumCast<u32>(rts.size()));

	mCmdList->OMSetRenderTargets(NumCast<u32>(rts.size()), &rtHandles[0], false, ds ? &dsHandle : nullptr);
}

void DX12Context::SetConstantBuffers(EShaderType shaderType, Span<IConstantBuffer* const> buffers)
{
	if (shaderType < EShaderType::GraphicsCount)
	{
		auto& targetVec = mGraphicsState.ShaderStates[shaderType].RootCBVs;
		targetVec.clear();
		for (IConstantBuffer* cb : buffers)
		{
			if (cb)
			{
				targetVec.push_back(static_cast<DX12DynamicCB*>(cb)->Commit());
			}
		}
	}
}

void DX12Context::SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const> cbs)
{
	if (shaderType < EShaderType::GraphicsCount)
	{
		auto& targetVec = mGraphicsState.ShaderStates[shaderType].RootCBVs;
		targetVec.clear();
		for (CBHandle const& cb : cbs)
		{
			targetVec.push_back(cb.UserData.As<D3D12_GPU_VIRTUAL_ADDRESS>());
		}
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

void DX12Context::Wait(OwningPtr<GPUSyncPoint>&& syncPoint)
{
	if (syncPoint == nullptr)
	{
		return;
	}

//	static_cast<DX12SyncPoint*>(&syncPoint)->GPUWait(m)
}

DX12Context::DX12Context(ID3D12GraphicsCommandList_* cmdList) :mCmdList(cmdList)
{
	Device = &GetRHI();
	CBPool = &GetRHI().GetCBPool();
	mDynamicCBs[ECBFrequency::VS_PerFrame].SetLayout(GetLayout<PerFrameVertexData>());
	mDynamicCBs[ECBFrequency::PS_PerFrame].GetCBData().Resize(MaxSize<PFPSPointLight, PFPSSpotLight, PerFramePSData>());
	mDynamicCBs[ECBFrequency::VS_PerInstance].GetCBData().Resize(sizeof(PerInstanceVSData));
	mDynamicCBs[ECBFrequency::PS_PerInstance].GetCBData().Resize(sizeof(PerInstancePSData));
}

void DX12Context::DrawMesh(IDeviceMesh const* mesh)
{
	FinalizeGraphicsState();
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (mesh->MeshType == EDeviceMeshType::DIRECT)
	{
		mCmdList->IASetVertexBuffers(0, 1, &static_cast<DX12DirectMesh const*>(mesh)->View);
		mCmdList->DrawInstanced(mesh->VertexCount, 1, 0, 0);
	}
	else
	{
		auto indexedMesh = static_cast<DX12IndexedMesh const*>(mesh);
		mCmdList->IASetVertexBuffers(0, 1, &indexedMesh->VertBuffView);
		mCmdList->IASetIndexBuffer(&indexedMesh->IndBuffView);
		mCmdList->DrawIndexedInstanced(indexedMesh->IndexCount, 1, 0, 0, 0);
	}
}

void DX12Context::Copy(DeviceResourceRef dst, DeviceResourceRef src)
{
	// Invalid cast, but ResourceType will always be in the same place
	switch (static_cast<IDeviceTexture*>(src.get())->Desc.ResourceType)
	{
	case EResourceType::Texture2D:
	{
		DX12Texture* src12 = static_cast<DX12Texture*>(src.get());
		DX12Texture* dst12 = static_cast<DX12Texture*>(dst.get());
		src12->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		dst12->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		RCHECK(dst12->Desc.ResourceType == EResourceType::Texture2D);
		mCmdList->CopyResource(dst12->GetTextureHandle<ID3D12Resource_*>(), src12->GetTextureHandle<ID3D12Resource_*>());
		break;
	}
	default:
		ZE_ENSURE(false);
		break;
	}
}

void DX12Context::ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour)
{
	static_cast<DX12RenderTarget*>(rt.get())->BaseTex->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCmdList->ClearRenderTargetView(rt->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>(), &clearColour[0], 0, nullptr);
}

void DX12Context::ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil /*= 0*/)
{
	static_cast<DX12DepthStencil*>(ds.get())->BaseTex->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	D3D12_CLEAR_FLAGS flags{};
	if (HasAnyFlags(clearMode, EDSClearMode::DEPTH))
	{
		flags |= D3D12_CLEAR_FLAG_DEPTH;
	}
	if (HasAnyFlags(clearMode, EDSClearMode::STENCIL))
	{
		flags |= D3D12_CLEAR_FLAG_STENCIL;
	}
	mCmdList->ClearDepthStencilView(ds->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>(), flags, depth, stencil, 0, nullptr);
}

}
