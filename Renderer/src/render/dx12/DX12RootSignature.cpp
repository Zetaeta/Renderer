#include "DX12RootSignature.h"
#include <d3dx12.h>
#include "DX12RHI.h"
#include "core/Logging.h"

namespace rnd::dx12
{

struct GRSSignature
{
	u32 VSCBs;
	u32 PSCBs;
	u32 VSUAVs;
	u32 PSUAVs;
	bool operator==(GRSSignature const& other) const
	{
		return memcmp(this, &other, sizeof(*this)) == 0;
	}
};

static std::unordered_map<GRSSignature, DX12GraphicsRootSignature, GenericHash<>> sRootSigs;

DX12GraphicsRootSignature DX12GraphicsRootSignature::MakeStandardRootSig(u32 numVertexCBs, u32 numPixelCBs, u32 vertexUAVs, u32 pixelUAVs)
{
	GRSSignature sigDesc {numVertexCBs, numPixelCBs, vertexUAVs, pixelUAVs};
	DX12GraphicsRootSignature& result = sRootSigs[sigDesc];
	if (result.Get() == nullptr)
	{
		Vector<D3D12_ROOT_PARAMETER> params(numVertexCBs + numPixelCBs + 2 );
		u32 offset = 0;

		constexpr static D3D12_SHADER_VISIBILITY ShaderVis[] = {D3D12_SHADER_VISIBILITY_VERTEX, D3D12_SHADER_VISIBILITY_PIXEL};
		D3D12_DESCRIPTOR_RANGE tableRanges[2][2];
		Zero(tableRanges);
		bool hasUavs[2] = {vertexUAVs > 0, pixelUAVs > 0};
		u32 uavs[2] = {vertexUAVs, pixelUAVs};
		result.NumUAVs[EShaderType::Vertex] = vertexUAVs;
		result.NumUAVs[EShaderType::Pixel] = pixelUAVs;
		for (u32 i = 0; i < 2; ++i)
		{
			if (hasUavs[i])
			{
				tableRanges[i][0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				tableRanges[i][0].NumDescriptors = i ? pixelUAVs : vertexUAVs;
			}
			tableRanges[i][hasUavs[i]].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			tableRanges[i][hasUavs[i]].OffsetInDescriptorsFromTableStart = uavs[i];
			tableRanges[i][hasUavs[i]].NumDescriptors = -1;
		}
		result.SRVTableIndices[EShaderType::Vertex] = offset;
		result.SRVTableIndices[EShaderType::Pixel] = offset + 1;
		//tableRanges[0]
		//tableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		//tableRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		//tableRanges[1].NumDescriptors = -1;
		for (int i = 0; i < ARRAY_SIZE(ShaderVis); ++i)
		{
			params[i + offset].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[i + offset].ShaderVisibility = ShaderVis[i];
			params[i + offset].DescriptorTable.NumDescriptorRanges = hasUavs[i] + 1;
			params[i + offset].DescriptorTable.pDescriptorRanges = tableRanges[i];
		}
		offset = 2;
		result.RootCBVIndices[EShaderType::Vertex] = offset;
		for (u32 i = 0; i < numVertexCBs; ++i)
		{
			params[i + offset].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			params[i + offset].Descriptor = {i, 0};
			params[i + offset].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		}

		offset += numVertexCBs;
		result.RootCBVIndices[EShaderType::Pixel] = offset;
		for (u32 i = 0; i < numPixelCBs; ++i)
		{
			params[i + offset].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			params[i + offset].Descriptor = {i, 0};
			params[i + offset].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}

		offset += numPixelCBs;

		//params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		//params[1].DescriptorTable.NumDescriptorRanges = 1;
		//D3D12_DESCRIPTOR_RANGE srvTable{};
		//srvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		//srvTable.NumDescriptors = -1;
		//params[1].DescriptorTable.pDescriptorRanges = &srvTable;
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {CD3DX12_STATIC_SAMPLER_DESC(0), CD3DX12_STATIC_SAMPLER_DESC(1)};
		samplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		samplerDesc[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
		rootSigDesc.Init(Size(params), Addr(params), 2, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		if (!SUCCEEDED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)) && error)
		{
			RLOG(LogGlobal, Error, "Root signature error: %s", error->GetBufferPointer());
			ZE_ENSURE(false);
		}
		DXCALL(GetD3D12Device()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&result.mRootSig)));

	}
	return result;
}

DX12GraphicsRootSignature::DX12GraphicsRootSignature()
{
	for (EShaderType shader = EShaderType::GraphicsStart; shader < EShaderType::GraphicsCount; ++shader)
	{
		RootCBVIndices[shader] = -1;
		SRVTableIndices[shader] = -1;
	}
}

void DX12GraphicsRootSignature::ClearCache()
{
	sRootSigs = {};
}

struct CRSSignature
{
	u32 numCBs;
	u32 numUAVs;
	bool operator==(CRSSignature const& other) const
	{
		return memcmp(this, &other, sizeof(*this)) == 0;
	}
};
static std::unordered_map<CRSSignature, DX12ComputeRootSignature, GenericHash<>> sComputeRootSigs;

DX12ComputeRootSignature DX12ComputeRootSignature::MakeStandardRootSig(u32 numCBs, u32 numUAVs /*= 0*/)
{
	DX12ComputeRootSignature& result = sComputeRootSigs[CRSSignature{numCBs, numUAVs}];
	if (result.Get() == nullptr)
	{
		SmallVector<D3D12_ROOT_PARAMETER, 4> params(numCBs + 1);
		u32 offset = 0;
		D3D12_DESCRIPTOR_RANGE tableRanges[2];
		Zero(tableRanges);
		bool hasUAVs = numUAVs > 0;
		if (hasUAVs)
		{
			tableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			tableRanges[0].NumDescriptors = numUAVs;
		}
		tableRanges[hasUAVs].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		tableRanges[hasUAVs].OffsetInDescriptorsFromTableStart = numUAVs;
		tableRanges[hasUAVs].NumDescriptors = -1;

		params[offset].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[offset].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		params[offset].DescriptorTable.NumDescriptorRanges = hasUAVs + 1;
		params[offset].DescriptorTable.pDescriptorRanges = tableRanges;
		offset = 1;
		for (u32 i = 0; i < numCBs; ++i)
		{
			params[i + offset].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			params[i + offset].Descriptor = {i, 0};
			params[i + offset].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		}
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3] = {CD3DX12_STATIC_SAMPLER_DESC(0), CD3DX12_STATIC_SAMPLER_DESC(1), CD3DX12_STATIC_SAMPLER_DESC(2)};
		samplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		samplerDesc[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;

		// Point clamped sampler
		{
			samplerDesc[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			samplerDesc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		
		}
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
		rootSigDesc.Init(Size(params), Addr(params), 2, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		if (!SUCCEEDED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)) && error)
		{
			RLOG(LogGlobal, Error, "Root signature error: %s", error->GetBufferPointer());
			ZE_ENSURE(false);
		}
		DXCALL(GetD3D12Device()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&result.mRootSig)));
		result.NumCBs = numCBs;
		result.NumUAVs = numUAVs;
	}
	return result;
}

void DX12ComputeRootSignature::ClearCache()
{
	sComputeRootSigs = {};
}

}
