#pragma once
#include "render/RenderDeviceCtx.h"
#include "SharedD3D12.h"
#include "DX12RootSignature.h"
#include "container/Vector.h"
#include "DX12CBPool.h"
#include "DX12State.h"

namespace rnd::dx12
{

class DX12Context : public IRenderDeviceCtx
{
public:
	DX12Context(DX12CommandList& cmdList, bool manualTransitioning = false);

	void SetRTAndDS(IRenderTarget::Ref rts, IDepthStencil::Ref ds, int RTArrayIdx, int DSArrayIdx) override;


	void SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds) override;


	void SetViewport(float width, float height, float TopLeftX, float TopLeftY) override;


	void SetDepthMode(EDepthMode mode) override
	{
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.DepthMode, mode);
	}


	void SetDepthStencilMode(EDepthMode mode, StencilState stencil ) override
	{
		SetDepthMode(mode);
		SetStencilState(stencil);
	}


	void SetStencilState(StencilState stencil) override;


	void SetBlendMode(EBlendState mode) override
	{
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.BlendState, mode);
	}


	void ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil = 0) override;


	void ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour) override;


	void ClearUAV(UnorderedAccessView uav, uint4 clearValues) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearUAV(UnorderedAccessView uav, vec4 clearValues) override
	{
//		throw std::logic_error("The method or operation is not implemented.");
	}


	void FinalizePipeline(EGpuPipeline pipeline) override
	{
		if (pipeline == EGpuPipeline::Graphics)
		{
			FinalizeGraphicsState();
		}
		else
		{
			FinalizeComputeState();
		}
	}
	void DrawMesh(IDeviceMesh const* mesh) override;


	void DispatchCompute(ComputeDispatch args) override;


	IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size = 0) override
	{
		if (size > mDynamicCBs[freq].GetCBData().GetSize())
		{
			mDynamicCBs[freq].GetCBData().Resize(size);
		}
		return &mDynamicCBs[freq];
	}


	void SetConstantBuffers(EShaderType shaderType, Span<IConstantBuffer* const> buffers) override;


	void SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const>) override;


	void UpdateConstantBuffer(CBHandle handle, std::span<const byte> data) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearResourceBindings() override
	{
//		throw std::logic_error("The method or operation is not implemented.");
	}


	void ResolveMultisampled(DeviceSubresource const& Dest, DeviceSubresource const& Src) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void Copy(DeviceResourceRef dst, DeviceResourceRef src) override;


	void SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx = 0) override;


	void SetUAVs(EShaderType shader, Span<UnorderedAccessView const> uavs, u32 startIdx = 0) override;

	void UnbindUAVs(EShaderType shader, u32 clearNum, u32 startIdx = 0) override
	{
//		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetSamplers(EShaderType shader, Span<SamplerHandle const> samplers, u32 startSlot = 0) override
	{
//		throw std::logic_error("The method or operation is not implemented.");
	}


	GPUTimer* CreateTimer(const wchar_t* Name) override;


	void StartTimer(GPUTimer* timer) override;


	void StopTimer(GPUTimer* timer) override;


	void SetPixelShader(PixelShader const* shader) override
	{
		SetGraphicsShader(EShaderType::Pixel, shader);
	}


	void SetVertexShader(VertexShader const* shader) override
	{
		SetGraphicsShader(EShaderType::Vertex, shader);
	}


	void SetComputeShader(ComputeShader const* shader) override
	{
		if (shader != mComputeState.PSODesc.Shader)
		{
			mComputeState.PSODesc.Shader = shader;
			mComputeState.PSODirty = true;
		}
	}


	void SetVertexLayout(VertAttDescHandle attDescHandle) override
	{
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.VertexLayoutHdl, attDescHandle);
	}


	MappedResource Readback(DeviceResourceRef resource, u32 subresource, _Out_opt_ RefPtr<GPUSyncPoint>* completionSyncPoint) override;
//j	void		   ReleaseReadback(MappedResource resource) override;

	void SetGraphicsRootSignature(DX12GraphicsRootSignature const& rootSig);

	void ExecuteCommands() override;
	void Wait(GPUSyncPoint* syncPoint);

	void SetGraphicsShader(EShaderType shaderType, Shader const* shader);

	void DirtyPSO()
	{
		mGraphicsState.PSODirty = true;
	}

	void DirtyBindings()
	{
		mGraphicsState.mBindingsDirty = true;
	}

	void FinalizeGraphicsState();
	void FinalizeComputeState();
	void SetupDescriptorTable(ShaderBindingState const& bindings, u32 rootArgument, DescriptorTableLoc& inoutLocation, u32 numUAVs);

protected:
	ID3D12CommandQueue* GetQueue();
	void BindAndTransition(Vector<UnorderedAccessView>& outBindings, Span<UnorderedAccessView const> inUAVs, u32 startIdx = 0);
	void BindAndTransition(Vector<ResourceView>& outBindings, Span<ResourceView const> inSRVs,
		D3D12_RESOURCE_STATES targetState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		u32 startIdx = 0);

	DX12CommandList& mCmdList;
	GraphicsPipelineStateCache mGraphicsState;
	ComputePipelineStateCache mComputeState;
	EnumArray<ECBFrequency, DX12DynamicCB> mDynamicCBs;
	bool mManualTransitioning = false;
};

}
