#pragma once
#include "render/RenderDeviceCtx.h"
#include "DX12RootSignature.h"
#include "container/Vector.h"

namespace rnd { namespace dx12 { struct DescriptorTableLoc; } }

namespace rnd::dx12
{

struct ShaderBindingState
{
	SmallVector<D3D12_GPU_VIRTUAL_ADDRESS, 2> RootCBVs;
	Vector<ResourceView> SRVs;
	Vector<UnorderedAccessView> UAVs;

	u32 GetRequiredDescriptors()
	{
		return NumCast<u32>(UAVs.size() + SRVs.size());
	}

//	void Apply(ID3D12GraphicsCommandList_* cmdList);
	bool DescTablesDirty  = true;
};

struct GraphicsPSODesc
{
	DX12GraphicsRootSignature RootSig;
	EnumArray<EShaderType, RefPtr<Shader const>, EShaderType::GraphicsCount> Shaders;
	VertexAttributeDesc::Handle VertexLayoutHdl;
	u32 NumRTs = 0;
	std::array<DXGI_FORMAT, 8> RTVFormats;
	DXGI_FORMAT DSVFormat;
	DXGI_SAMPLE_DESC SampleDesc {1, 0};
	EBlendState BlendState;
	EDepthMode DepthMode;
	EStencilMode StencilMode;

	GraphicsPSODesc()
	{
		std::fill(RTVFormats.begin(), RTVFormats.end(), DXGI_FORMAT_UNKNOWN);
	}

	bool operator==(GraphicsPSODesc const& other) const
	{
		return memcmp(this, &other, sizeof(GraphicsPSODesc)) == 0;
	}
};

template<typename Hasher>
void HashAppend(Hasher& hasher, GraphicsPSODesc const& psoDesc)
{
	HashAppend(hasher, psoDesc.RootSig.Get());
	hasher(&psoDesc.Shaders, sizeof(psoDesc.Shaders));
	HashAppend(hasher, psoDesc.VertexLayoutHdl);
	HashAppend(hasher, psoDesc.RTVFormats);
	HashAppend(hasher, psoDesc.DSVFormat);
	HashAppend(hasher, psoDesc.SampleDesc);
	HashAppend(hasher, psoDesc.BlendState);
	HashAppend(hasher, psoDesc.DepthMode);
}

struct GraphicsPipelineStateCache
{
public:
	EnumArray<EShaderType, ShaderBindingState, EShaderType::GraphicsCount> ShaderStates;
	u32 ShaderMask = 0;

	struct ActiveStageIterator
	{
		ActiveStageIterator(GraphicsPipelineStateCache& parent)
		:mStateCache(parent)
		{
		}
		EShaderType Shader = EShaderType::GraphicsStart;
		ShaderBindingState* operator->() const
		{
			return &mStateCache.ShaderStates[Shader];
		}

		operator bool() const
		{
			return Shader < EShaderType::GraphicsCount;
		}

		ActiveStageIterator& operator++()
		{
			do
			{
				++Shader;
			}
			while (Shader < EShaderType::GraphicsCount && mStateCache.IsActive(Shader));
			return *this;
		}
		GraphicsPipelineStateCache& mStateCache;
	};

	ActiveStageIterator BeginActiveStages()
	{
		return ActiveStageIterator(*this);
	}

	DX12GraphicsRootSignature& RootSig()
	{
		return PSOState.RootSig;
	}

	auto& Shaders()
	{
		return PSOState.Shaders;
	}

	template<typename T>
	bool UpdatePSO(T& currentState, T const& newState)
	{
		if (currentState != newState)
		{
			currentState = newState;
			PSODirty = true;
			return true;
		}
		return false;
	}

	GraphicsPSODesc PSOState;

	bool PSODirty = true;
	bool mBindingsDirty = true;
	bool RootSigDirty = true;

	bool IsActive(EShaderType stage) const
	{
		return (1 << Denum(stage)) & ShaderMask;
	}

	void SetActive(EShaderType stage)
	{
		ShaderMask &= (1 << Denum(stage));
	}
};

struct ComputePSODesc
{
	DX12ComputeRootSignature RootSig;
	RefPtr<ComputeShader const> Shader;

	bool operator==(ComputePSODesc const& other) const
	{
		return memcmp(this, &other, sizeof(*this)) == 0;
	}
};
template<typename Hasher>
void HashAppend(Hasher& hasher, ComputePSODesc const& psoDesc)
{
	HashAppend(hasher, psoDesc.RootSig.Get());
	HashAppend(hasher, psoDesc.Shader.Get());
}

struct ComputePipelineStateCache
{
	ComputePSODesc PSODesc;
	ShaderBindingState Bindings;
	bool PSODirty = true;
	bool BindingsDirty = true;
	bool RootSigDirty = true;

	template<typename T>
	bool UpdatePSO(T& currentState, T const& newState)
	{
		if (currentState != newState)
		{
			currentState = newState;
			PSODirty = true;
			return true;
		}
		return false;
	}
};

class DX12Context : public IRenderDeviceCtx
{
public:
	DX12Context(ID3D12GraphicsCommandList_* cmdList, bool manualTransitioning = false);

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


	void SetStencilState(StencilState stencil) override
	{
		mGraphicsState.UpdatePSO(mGraphicsState.PSOState.StencilMode, stencil.Mode);
		mCmdList->OMSetStencilRef(stencil.WriteValue);
	}


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


	void DrawCubemap(IDeviceTextureCube* cubemap) override
	{
		FinalizeGraphicsState();
//		throw std::logic_error("The method or operation is not implemented.");
	}



	void DrawMesh(Primitive const& primitive) override
	{
		FinalizeGraphicsState();
//		throw std::logic_error("The method or operation is not implemented.");
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


	GPUTimer* CreateTimer(const wchar_t* Name) override
	{
		return nullptr;
	}


	void StartTimer(GPUTimer* timer) override
	{
//		throw std::logic_error("The method or operation is not implemented.");
	}


	void StopTimer(GPUTimer* timer) override
	{
//		throw std::logic_error("The method or operation is not implemented.");
	}


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


	void PrepareMaterial(MaterialID mid) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void SetGraphicsRootSignature(DX12GraphicsRootSignature const& rootSig)
	{
		if (mGraphicsState.UpdatePSO(mGraphicsState.RootSig(), rootSig))
		{
			mCmdList->SetGraphicsRootSignature(rootSig.Get());
		}
	}

	void Wait(OwningPtr<GPUSyncPoint>&& syncPoint);

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
	void BindAndTransition(Vector<UnorderedAccessView>& outBindings, Span<UnorderedAccessView const> inUAVs, u32 startIdx = 0);
	void BindAndTransition(Vector<ResourceView>& outBindings, Span<ResourceView const> inSRVs,
		D3D12_RESOURCE_STATES targetState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		u32 startIdx = 0);

	ID3D12GraphicsCommandList_* mCmdList;
	GraphicsPipelineStateCache mGraphicsState;
	ComputePipelineStateCache mComputeState;
	EnumArray<ECBFrequency, DX12DynamicCB> mDynamicCBs;
	bool mManualTransitioning = false;
};

}
