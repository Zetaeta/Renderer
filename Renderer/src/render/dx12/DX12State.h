#pragma once
#include "SharedD3D12.h"
#include "container/Vector.h"
#include "container/EnumArray.h"
#include "render/Shader.h"
#include "DX12RootSignature.h"
#include "render/RenderState.h"

namespace rnd { namespace dx12 { struct DX12CommandList; } }

namespace rnd { namespace dx12 { struct DescriptorTableLoc; } }

namespace rnd::dx12
{

enum class EDescriptorType : u8
{
	SRV,
	UAV,
	CBV,
	Sampler,
	RTV,
	DSV,

	Count,
	ShaderVisibleCount = Sampler + 1,
	CbvSrvUavCount = CBV + 1,

};

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
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
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
}
