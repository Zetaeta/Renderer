#include "DX12Context.h"
#include "DX12RHI.h"
#include <d3dx12.h>
#include "DX12DescriptorTableAllocator.h"
#include "DX12Texture.h"
#include "render/dxcommon/DXGIUtils.h"
#include "render/RenderContext.h"
#include "render/ShadingCommon.h"
#include "pix3.h"
#include "DX12ShaderCompiler.h"

namespace rnd::dx12
{

void DX12Context::SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx /*= 0*/)
{
	if (shader < EShaderType::GraphicsCount)
	{
		//.clear();
		//mGraphicsState.ShaderStates[shader].SRVs.insert(mGraphicsState.ShaderStates[shader].SRVs.end(), srvs.begin(), srvs.end());
		mGraphicsState.ShaderStates[shader].DescTablesDirty = true;
		mGraphicsState.mBindingsDirty = true;
		auto requiredState = shader == EShaderType::Pixel ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		BindAndTransition(mGraphicsState.ShaderStates[shader].SRVs, srvs, requiredState, startIdx);
		//for (auto const& srv : srvs)
		//{
		//	if (srv.Resource)
		//	{
		//		static_cast<DX12Texture*>(srv.Resource.get())->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		//	}
		//}
	}
	else
	{
		ZE_ASSERT(shader == EShaderType::Compute);
		mComputeState.BindingsDirty = true;
		mComputeState.Bindings.DescTablesDirty = true;
		BindAndTransition(mComputeState.Bindings.SRVs, srvs, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, startIdx);
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
			mCmdList->SetGraphicsRootConstantBufferView(mGraphicsState.RootSig().RootCBVIndices[stage.Shader] + i, stage->RootCBVs[i]);
		}
	}

	if (!bResourcesDirty)
	{
		return;
	}

	if (requiredResourceDescriptors > 0)
	{
		auto loc = rhi.GetResourceDescTableAlloc()->Reserve(NumCast<u32>(requiredResourceDescriptors));
		mCmdList->SetDescriptorHeaps(1, &loc.Heap);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dest(loc.CPUHandle);
		CD3DX12_GPU_DESCRIPTOR_HANDLE tableStart(loc.GPUHandle);
		for (auto stage = mGraphicsState.BeginActiveStages(); stage; ++stage)
		{
			if (stage->GetRequiredDescriptors() > 0)
			{
				u32 tableSize = 0;
				auto numUAVs = min(stage->UAVs.size(), (u64) mGraphicsState.PSOState.RootSig.NumUAVs[stage.Shader]);
				for (u32 i=0; i<numUAVs; ++i)
				{
					UnorderedAccessView& view = stage->UAVs[i];
					auto src = view.Resource->GetUAV<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId);
					rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
				}
				for (ResourceView& view : stage->SRVs)
				{
					auto rsc = view.Resource ? view.Resource : GetRHI().BasicTextures.GetBlackTexture(&GetRHI());
					auto src = rsc->GetShaderResource<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId);
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
				tableStart.Offset(NumCast<u32>(stage->SRVs.size() + stage->UAVs.size()), DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
			}
		}
	}
}
void DX12Context::SetupDescriptorTable(ShaderBindingState const& bindings, u32 rootArgument, DescriptorTableLoc& inoutLocation, u32 numUAVs)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(inoutLocation.CPUHandle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE tableStart(inoutLocation.GPUHandle);
	auto& rhi = GetRHI();
	numUAVs = min(numUAVs, (u32)bindings.UAVs.size());
	for (u32 i=0; i<numUAVs; ++i)
	{
		UnorderedAccessView const& view = bindings.UAVs[i];
		D3D12_CPU_DESCRIPTOR_HANDLE src {view.Resource->GetUAV<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId)};
		rhi.Device()->CopyDescriptorsSimple(1, dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
	}
	for (ResourceView const& view : bindings.SRVs)
	{
		auto rsc = view.Resource ? view.Resource : GetRHI().BasicTextures.GetBlackTexture(&GetRHI());
		auto src = rsc->GetShaderResource<D3D12_CPU_DESCRIPTOR_HANDLE>(view.ViewId);
		rhi.Device()->CopyDescriptorsSimple(1,dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		dest.Offset(DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
	}
	tableStart.Offset(NumCast<u32>(bindings.SRVs.size() + bindings.UAVs.size()), DX12DescriptorHeap::DescriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
	inoutLocation.CPUHandle = dest;
	inoutLocation.GPUHandle = tableStart;
}


void DX12Context::FinalizeComputeState()
{
	auto& rhi = GetRHI();
	if (mComputeState.PSODirty)
	{
		mComputeState.RootSigDirty = SetWasChanged(mComputeState.PSODesc.RootSig,
			DX12ComputeRootSignature::MakeStandardRootSig(NumCast<u32>(mComputeState.Bindings.RootCBVs.size()),
														  NumCast<u32>(mComputeState.Bindings.UAVs.size())));
		mCmdList->SetPipelineState(rhi.GetPSO(mComputeState.PSODesc));
//		mComputeState.PSODirty = false;
		mComputeState.RootSigDirty = true;
	}
	if (mComputeState.RootSigDirty)
	{
		mCmdList->SetComputeRootSignature(mComputeState.PSODesc.RootSig.Get());
//		mComputeState.RootSigDirty = false;
	}

	for (u32 i=0; i<mComputeState.Bindings.RootCBVs.size(); ++i)
	{
		mCmdList->SetComputeRootConstantBufferView(mComputeState.PSODesc.RootSig.CBVStartIndex + i, mComputeState.Bindings.RootCBVs[i]);
	}

	if (mComputeState.BindingsDirty)
	{
		auto loc = rhi.GetResourceDescTableAlloc()->Reserve(NumCast<u32>(mComputeState.Bindings.UAVs.size() + mComputeState.Bindings.SRVs.size()));
		mCmdList->SetDescriptorHeaps(1, &loc.Heap);
		mCmdList->SetComputeRootDescriptorTable(mComputeState.PSODesc.RootSig.SRVTableIndex, loc.GPUHandle);
		SetupDescriptorTable(mComputeState.Bindings, mComputeState.PSODesc.RootSig.SRVTableIndex, loc, mComputeState.PSODesc.RootSig.NumUAVs);
		mComputeState.BindingsDirty = false;
	}
}


void DX12Context::SetRTAndDS(IRenderTarget::Ref rts, IDepthStencil::Ref ds, int RTArrayIdx, int DSArrayIdx)
{
	IRenderTarget::Ref adjustedRT{rts.RT, NumCast<u32>(max(RTArrayIdx, 0))};
	SetRTAndDS(rts? Single<IRenderTarget::Ref>(adjustedRT) : Span<IRenderTarget::Ref>{(IRenderTarget::Ref*)nullptr, 0}, {ds.DS, NumCast<u32>(max(DSArrayIdx, 0))});
}

void DX12Context::SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds)
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> rtHandles {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = {};
	if (ds)
	{
		dsHandle = static_cast<DX12DepthStencil*>(ds.get())->GetHandle(ds.Index);
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
			rtHandles[i] = rt->DescriptorHdl[rts[i].Index];
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
	auto& targetVec = shaderType < EShaderType::GraphicsCount ? mGraphicsState.ShaderStates[shaderType].RootCBVs : mComputeState.Bindings.RootCBVs;
	targetVec.clear();
	for (IConstantBuffer* cb : buffers)
	{
		if (cb)
		{
			targetVec.push_back(static_cast<DX12DynamicCB*>(cb)->Commit());
		}
	}
}

void DX12Context::SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const> cbs)
{
	auto& targetVec = shaderType < EShaderType::GraphicsCount ? mGraphicsState.ShaderStates[shaderType].RootCBVs : mComputeState.Bindings.RootCBVs;
	targetVec.clear();
	for (CBHandle const& cb : cbs)
	{
		targetVec.push_back(cb.UserData.As<D3D12_GPU_VIRTUAL_ADDRESS>());
	}

}

void DX12Context::SetGraphicsShader(EShaderType shaderType, Shader const* shader)
{
//	ZE_ASSERT(dynamic_cast<DX12Shader>(shader->GetDeviceShader()) != nullptr);
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

void DX12Context::Wait(GPUSyncPoint* syncPoint)
{
	if (syncPoint == nullptr)
	{
		return;
	}

	static_cast<DX12SyncPoint*>(syncPoint)->GPUWait(GetRHI().CmdQueues().Direct);
}

DX12Context::DX12Context(DX12CommandList& cmdList, bool manualTransitioning)
	: mCmdList(cmdList), mManualTransitioning(manualTransitioning)
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
	mCmdList->IASetPrimitiveTopology(GetD3D12Topology(mesh->Topology));

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
	if (!rt)
	{
		return;
	}
	static_cast<DX12RenderTarget*>(rt.get())->BaseTex->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (rt->Desc.Dimension == ETextureDimension::TEX_2D)
	{
		mCmdList->ClearRenderTargetView(rt->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>(), &clearColour[0], 0, nullptr);
	}
	else
	{
		for (u32 i = 0; i < 6; ++i)
		{
			mCmdList->ClearRenderTargetView(static_cast<DX12RenderTarget*>(rt.get())->GetHandle(i), &clearColour[0], 0, nullptr);
		}
	}
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
	if (ds->Desc.Dimension == ETextureDimension::TEX_2D)
	{
		mCmdList->ClearDepthStencilView(ds->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>(), flags, depth, stencil, 0, nullptr);
	}
	else
	{
		for (u32 i = 0; i < 6; ++i)
		{
			mCmdList->ClearDepthStencilView(static_cast<DX12DepthStencil*>(ds.get())->GetHandle(i), flags, depth, stencil, 0, nullptr);
		}
	}
}

void DX12Context::SetUAVs(EShaderType shader, Span<UnorderedAccessView const> uavs, u32 startIdx /*= 0*/)
{
	if (shader == EShaderType::Compute)
	{
		mComputeState.BindingsDirty = true;
		mComputeState.Bindings.DescTablesDirty = true;
		BindAndTransition(mComputeState.Bindings.UAVs, uavs);
	}
	else
	{
		mGraphicsState.mBindingsDirty = true;
		mGraphicsState.ShaderStates[shader].DescTablesDirty = true;
		BindAndTransition(mGraphicsState.ShaderStates[shader].UAVs, uavs, startIdx);
	}
}

void DX12Context::BindAndTransition(Vector<UnorderedAccessView>& outBindings, Span<UnorderedAccessView const> inUAVs, u32 startIdx)
{
	outBindings.clear();
	Append(outBindings, inUAVs);
	if (!mManualTransitioning)
	{
		for (auto const& uav : inUAVs)
		{
			if (DX12Texture* tex = static_cast<DX12Texture*>(uav.Resource.get()); tex && uav.ViewId == 0) // we're not doing subresource tracking yet
			{
				tex->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
		}
	}
}

void DX12Context::BindAndTransition(Vector<ResourceView>& outBindings, Span<ResourceView const> inSRVs, D3D12_RESOURCE_STATES targetState, u32 startIdx /*= 0*/)
{
	outBindings.clear();
	outBindings.resize(max(outBindings.size(), startIdx + inSRVs.size()));

	for (u32 i = 0; i < inSRVs.size(); ++i)
	{
		outBindings[startIdx + i] = inSRVs[i];
	}
	if (!mManualTransitioning)
	{
		for (auto const& srv : inSRVs)
		{
			if (DX12Texture* tex = static_cast<DX12Texture*>(srv.Resource.get()))
			{
//				ZE_ASSERT(srv.ViewId == 0);
				tex->TransitionTo(mCmdList, targetState);
			}
		}
	}
}

void DX12Context::DispatchCompute(ComputeDispatch args)
{
	FinalizeComputeState();
	mCmdList->Dispatch(args.ThreadGroupsX, args.ThreadGroupsY, args.ThreadGroupsZ);
}

void DX12Context::ExecuteCommands()
{
	mCmdList->Close();
	ID3D12CommandList* const cmdList = mCmdList.Get();
	GetQueue()->ExecuteCommandLists(1, &cmdList);
	mCmdList.Reset(GetRHI().GetDefaultPSO());
}

ID3D12CommandQueue* DX12Context::GetQueue()
{
	return GetRHI().CmdQueues().Direct;
}

void DX12Context::SetStencilState(StencilState stencil)
{
	mGraphicsState.UpdatePSO(mGraphicsState.PSOState.StencilMode, stencil.Mode);
	mCmdList->OMSetStencilRef(stencil.WriteValue);
}

void DX12Context::SetGraphicsRootSignature(DX12GraphicsRootSignature const& rootSig)
{
	if (mGraphicsState.UpdatePSO(mGraphicsState.RootSig(), rootSig))
	{
		mCmdList->SetGraphicsRootSignature(rootSig.Get());
	}
}

rnd::MappedResource DX12Context::Readback(DeviceResourceRef resource, u32 subresource, _Out_opt_ RefPtr<GPUSyncPoint>* completionSyncPoint)
{
	auto& rhi = GetRHI();
	ID3D12Resource_* dxResource = resource->GetRHIResource().As<ID3D12Resource_*>();
	auto desc = dxResource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	MappedResource result;
	u32 numRows;
	u64 rowSize;
	u64 totalBytes;
	rhi.Device()->GetCopyableFootprints(&desc, subresource, 1, 0, &footprint, &numRows, &rowSize, &totalBytes);
	result.RowPitch = footprint.Footprint.RowPitch;
	auto readbackBuffer = rhi.AllocateReadbackBuffer(totalBytes);
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		mCmdList->CopyBufferRegion(readbackBuffer.Resource.Get(), readbackBuffer.Offset, dxResource, footprint.Offset, footprint.Footprint.Width);
	}
	else
	{
		static_cast<DX12Texture*>(resource.get())->TransitionTo(mCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CD3DX12_TEXTURE_COPY_LOCATION const dst(readbackBuffer.Resource.Get(), footprint);
		CD3DX12_TEXTURE_COPY_LOCATION const src(dxResource, subresource);
		mCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}
	if (completionSyncPoint)
	{
		auto syncPoint = new DX12SyncPoint(DX12SyncPointPool::Get()->Claim());
		*completionSyncPoint = syncPoint;
		ExecuteCommands();
		syncPoint->GPUSignal(GetQueue());
	}
	// else ???

	DXCALL(readbackBuffer.Resource->Map(subresource, nullptr, &result.Data));
	result.Release = [readbackBuffer, subresource] () mutable
	{
		readbackBuffer.Resource->Unmap(subresource, nullptr);
		GetRHI().FreeReadback(readbackBuffer);
	};
	return result;
}

rnd::GPUTimer* DX12Context::CreateTimer(const wchar_t* Name)
{
	return GetRHI().CreateTimer(Name);
}

void MarkTimer(ID3D12GraphicsCommandList_* cmdList, DX12TimingQuery& timer)
{
	auto query = GetRHI().GetTimingQueries()->GetQuery();
	cmdList->EndQuery(query.Heap, D3D12_QUERY_TYPE_TIMESTAMP, query.Index);
	auto readbackAllocation = GetRHI().AllocateReadback(8, 8, 1);

	// TODO: batch these at end of frame (issue is resizing query heap mid frame)
	cmdList->ResolveQueryData(query.Heap, D3D12_QUERY_TYPE_TIMESTAMP, query.Index, 1, readbackAllocation.Buffer, readbackAllocation.BufferOffset); 
	timer.Location = reinterpret_cast<u64 const*>(readbackAllocation.Data);
}

void DX12Context::StartTimer(GPUTimer* timer)
{
	if (!timer)
	{
		return;
	}
		//	mCmdList->BeginEvent()
	auto myTimer = static_cast<DX12Timer*>(timer);
	PIXBeginEvent(mCmdList.Get(), 0, myTimer->Name.c_str());
	MarkTimer(mCmdList.Get(), myTimer->PerFrameData[GetRHI().GetCurrentFrameIndex()].Start);
}

void DX12Context::StopTimer(GPUTimer* timer)
{
	if (!timer)
	{
		return;
	}
	auto myTimer = static_cast<DX12Timer*>(timer);
	MarkTimer(mCmdList.Get(), myTimer->PerFrameData[GetRHI().GetCurrentFrameIndex()].End);
	PIXEndEvent(mCmdList.Get());
}

//void DX12Context::ReleaseReadback(MappedResource resource)
//{
//}

}
