#pragma once
#include "SharedD3D12.h"
#include "render/Shader.h"
#include "container/EnumArray.h"

namespace rnd::dx12
{

class DX12RootSignature
{
public:
	ID3D12RootSignature* Get() const
	{
		return mRootSig.Get();
	}
	bool operator==(DX12RootSignature const& other)
	{
		return mRootSig == other.mRootSig;
	}
protected:
	ComPtr<ID3D12RootSignature> mRootSig;
};

template<typename T>
using GraphicsStageArray = EnumArray<EShaderType, T, EShaderType::GraphicsCount>;

class DX12GraphicsRootSignature : public DX12RootSignature
{
public:
	bool operator==(DX12GraphicsRootSignature const& other) const
	{
		return mRootSig == other.mRootSig;
	}
	DX12GraphicsRootSignature();
	//u8 VertexCBStart = 0;
	//u8 PixelCBStart = 0;
	GraphicsStageArray<s8> RootCBVIndices;
	GraphicsStageArray<u8> NumUAVs {};
	//u8 PixelSRVTable = 0;
	//u8 VertexSRVTable = 0;
	GraphicsStageArray<s8> SRVTableIndices;
	//u8 PixelUAVTable = 0;
	//u8 VertexUAVTable = 0;
	static DX12GraphicsRootSignature MakeStandardRootSig(u32 numVertexCBs, u32 numPixelCBs, u32 vertexUAVs = 0, u32 pixelUAVs = 0);
	static void						 ClearCache();
};

class DX12ComputeRootSignature : public DX12RootSignature
{
public:
	DX12ComputeRootSignature() {}
	// Standard layout: 1 SRV/UAV descriptor table starting with UAVs, followed by 1 or more CBVs
	u32 NumUAVs = 0;
	u32 NumCBs = 0;
	constexpr static u32 SRVTableIndex = 0;
	constexpr static u32 CBVStartIndex = 1;
	static DX12ComputeRootSignature MakeStandardRootSig(u32 numCBs, u32 numUAVs = 0);
	static void						 ClearCache();
};

}
