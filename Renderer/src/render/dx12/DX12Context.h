#pragma once
#include "render/RenderDeviceCtx.h"

namespace rnd::dx12
{

class DX12Context : public IRenderDeviceCtx
{
public:
	DX12Context(ID3D12GraphicsCommandList_* cmdList)
	:mCmdList(cmdList) { }
	void SetRTAndDS(IRenderTarget::Ref rts, IDepthStencil::Ref ds, int RTArrayIdx, int DSArrayIdx) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetViewport(float width, float height, float TopLeftX, float TopLeftY) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetDepthMode(EDepthMode mode) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetDepthStencilMode(EDepthMode mode, StencilState stencil ) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetStencilState(StencilState stencil) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetBlendMode(EBlendState mode) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil = 0) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearUAV(UnorderedAccessView uav, uint4 clearValues) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearUAV(UnorderedAccessView uav, vec4 clearValues) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void DrawCubemap(IDeviceTextureCube* cubemap) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void DrawMesh(MeshPart const& meshPart, EShadingLayer layer, bool useMaterial) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void DrawMesh(Primitive const& primitive) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void DrawMesh(IDeviceMesh* mesh) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void DispatchCompute(ComputeDispatch args) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size = 0) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetConstantBuffers(EShaderType shaderType, IConstantBuffer** buffers, u32 numBuffers) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const>) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void UpdateConstantBuffer(CBHandle handle, std::span<const byte> data) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ClearResourceBindings() override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void ResolveMultisampled(DeviceSubresource const& Dest, DeviceSubresource const& Src) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void Copy(DeviceResourceRef dst, DeviceResourceRef src) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx = 0) override;


	void SetUAVs(EShaderType shader, Span<UnorderedAccessView const> uavs, u32 startIdx = 0) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void UnbindUAVs(EShaderType shader, u32 clearNum, u32 startIdx = 0) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetSamplers(EShaderType shader, Span<SamplerHandle const> samplers, u32 startSlot = 0) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	GPUTimer* CreateTimer(const wchar_t* Name) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void StartTimer(GPUTimer* timer) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void StopTimer(GPUTimer* timer) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetPixelShader(PixelShader const* shader) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetVertexShader(VertexShader const* shader) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetComputeShader(ComputeShader const* shader) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void SetVertexLayout(VertAttDescHandle attDescHandle) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void PrepareMaterial(MaterialID mid) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	ID3D12GraphicsCommandList_* mCmdList;
};

}
