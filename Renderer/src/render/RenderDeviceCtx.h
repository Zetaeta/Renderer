#pragma once
#include "core/BaseDefines.h"
#include "render/Renderer.h"
#include <core/Utils.h>
#include "DeviceTexture.h"
#include <common/Material.h>
#include "Shader.h"
#include "GPUTimer.h"
#include "RenderTextureManager.h"
#include "RenderDevice.h"
#include "RenderState.h"


struct MeshPart;

enum class EShadingLayer : u8;

namespace rnd
{
enum class EShaderType : u8;

using Primitive = MeshPart;

struct ComputeDispatch
{
	ComputeDispatch() = default;
	ComputeDispatch(uvec2 xy, u32 z = 1)
		: ThreadGroupsX(xy.x), ThreadGroupsY(xy.y), ThreadGroupsZ(z) {}
	ComputeDispatch(uvec3 xyz)
		: ThreadGroupsX(xyz.x), ThreadGroupsY(xyz.y), ThreadGroupsZ(xyz.z) {}
	ComputeDispatch(u32 x, u32 y, u32 z)
		: ThreadGroupsX(x), ThreadGroupsY(y), ThreadGroupsZ(z) {}
	u32 ThreadGroupsX = 1;
	u32 ThreadGroupsY = 1;
	u32 ThreadGroupsZ = 1;
};

class IRenderDeviceCtx
{
public:
	

	virtual void SetRTAndDS(IRenderTarget::Ref rts, IDepthStencil::Ref ds, int RTArrayIdx = -1, int DSArrayIdx = -1) = 0;
	virtual void SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds) = 0;
	virtual void SetViewport(float width, float height, float TopLeftX = 0, float TopLeftY = 0) = 0;
	inline void SetViewport(u32 width, u32 height, u32 TopLeftX = 0, u32 TopLeftY = 0)
	{
		SetViewport(float(width), float(height), float(TopLeftX), float(TopLeftY));
	}
	virtual void SetDepthMode(EDepthMode mode) = 0;
	virtual void SetDepthStencilMode(EDepthMode mode, StencilState stencil = {}) = 0;
	virtual void SetStencilState(StencilState stencil) = 0;
	virtual void SetBlendMode(EBlendState mode) = 0;
	virtual void ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil = 0) = 0;
	virtual void ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour) = 0;
	virtual void ClearUAV(UnorderedAccessView uav, uint4 clearValues) = 0;
	virtual void ClearUAV(UnorderedAccessView uav, vec4 clearValues) = 0;
//	virtual void DrawMesh(Primitive const& primitive) = 0;
	virtual void FinalizePipeline(EGpuPipeline Pipeline) {}
	virtual void DrawMesh(IDeviceMesh const* mesh) = 0;
	virtual void DispatchCompute(ComputeDispatch args) = 0;
	virtual IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size = 0) = 0; 
	virtual void			 SetConstantBuffers(EShaderType shaderType, Span<IConstantBuffer* const> buffers) = 0;
	virtual void			 SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const>) = 0;

	virtual MappedResource Readback(DeviceResourceRef resource, u32 subresource, _Out_opt_ RefPtr<GPUSyncPoint>* completionSyncPoint) = 0;
//	void				   ReleaseReadback(MappedResource resource) = 0;

	virtual void	 UpdateConstantBuffer(CBHandle handle, std::span<const byte> data) = 0;
	template<typename T>
	void UpdateConstantBuffer(CBHandle handle, T const& data)
	{
		Span<const byte> bytes(reinterpret_cast<const byte*>(&data), sizeof(data));
		UpdateConstantBuffer(handle, bytes);
	}

	virtual void ClearResourceBindings() = 0;

	virtual void ResolveMultisampled(DeviceSubresource const& Dest, DeviceSubresource const& Src) = 0;
	virtual void Copy(DeviceResourceRef dst, DeviceResourceRef src) = 0;

	virtual void SetShaderResources(EShaderType shader, ShaderResources srvs, u32 startIdx = 0) = 0;
	virtual void SetUAVs(EShaderType shader, Span<UnorderedAccessView const> uavs, u32 startIdx = 0) = 0;
	virtual void UnbindUAVs(EShaderType shader, u32 clearNum, u32 startIdx = 0) = 0;
	virtual void SetSamplers(EShaderType shader, Span<SamplerHandle const> samplers, u32 startSlot = 0) = 0;
	virtual void Wait(GPUSyncPoint* syncPoint) = 0;
	virtual void ExecuteCommands() {}

#if PROFILING
	virtual GPUTimer* CreateTimer(const wchar_t* Name) = 0;
	virtual void StartTimer(GPUTimer* timer) = 0;
	virtual void StopTimer(GPUTimer* timer) = 0;
#endif


	virtual void SetPixelShader(PixelShader const* shader) = 0;
	virtual void SetVertexShader(VertexShader const* shader) = 0;
	virtual void SetComputeShader(ComputeShader const* shader) = 0;

	virtual void SetVertexLayout(VertAttDescHandle attDescHandle) = 0;

	virtual ~IRenderDeviceCtx() {}

	ICBPool* CBPool = nullptr;
	IRenderDevice* Device;
//	class MaterialManager* MatManager;
};

}

