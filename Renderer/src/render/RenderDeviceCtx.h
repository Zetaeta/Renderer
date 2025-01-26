#pragma once
#include "core/BaseDefines.h"
#include "render/Renderer.h"
#include <core/Utils.h>
#include "DeviceTexture.h"
#include <common/Material.h>
#include "Shader.h"
#include "GPUTimer.h"
#include "RenderTextureManager.h"


struct MeshPart;
class GPUSyncPoint;
namespace rnd { class IConstantBuffer; }
namespace rnd { class IRenderDevice; }

enum class EShadingLayer : u8;

namespace rnd
{
enum class ECBFrequency : u8;
enum class EBlendState : u32
{
	NONE = 0,
	COL_OVERWRITE = 0x1,
	COL_ADD = 0x2,
	COL_BLEND_ALPHA = 0x4,
	COL_OBSCURE_ALPHA = 0x8,
	ALPHA_OVERWRITE = 0x10,
	ALPHA_ADD = 0x20,
	ALPHA_UNTOUCHED = 0x40,
	ALPHA_MAX = 0x80
};

FLAG_ENUM(EBlendState)

constexpr EBlendState BS_OPAQUE = EBlendState::COL_OVERWRITE
								| EBlendState::ALPHA_OVERWRITE;
constexpr EBlendState BS_OPAQUE_LAYER = EBlendState::COL_ADD
									  | EBlendState::ALPHA_MAX;
constexpr EBlendState BS_TRANSLUCENT = EBlendState::COL_BLEND_ALPHA
									 | EBlendState::COL_OBSCURE_ALPHA
									 | EBlendState::ALPHA_ADD;
constexpr EBlendState BS_TRANSLUCENT_LAYER = EBlendState::COL_BLEND_ALPHA
										   | EBlendState::ALPHA_ADD;

enum class EDepthMode : u8
{
	Disabled = 0,
	Less = 1,
	LessEqual = 2,
	Equal = 3,
	GreaterEqual = 4,
	Greater = 5,
	NoWrite = 0x08,
	Count = 14
};

FLAG_ENUM(EDepthMode);


enum class EStencilMode : u8
{
	Disabled = 0,
	Overwrite = 1,
	IgnoreDepth = 0x02,
	UseBackFace = 0x04,
	Count = 8
};

FLAG_ENUM(EStencilMode);
;
struct StencilState
{
	EStencilMode Mode = EStencilMode::Disabled;
	u8 WriteValue = 0;
};

enum class EDSClearMode : u8
{
	DEPTH = 0x1,
	STENCIL = 0x2,
	DEPTH_STENCIL = 0x3
};

FLAG_ENUM(EDSClearMode);

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
	virtual void DrawCubemap(IDeviceTextureCube* cubemap) = 0;
	virtual void DrawMesh(Primitive const& primitive) = 0;
	virtual void DrawMesh(IDeviceMesh const* mesh) = 0;
	virtual void DispatchCompute(ComputeDispatch args) = 0;
	virtual IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size = 0) = 0; 
	virtual void			 SetConstantBuffers(EShaderType shaderType, Span<IConstantBuffer* const> buffers) = 0;
	virtual void			 SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const>) = 0;

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
	virtual void Wait(OwningPtr<GPUSyncPoint>&& syncPoint) = 0;

#if PROFILING
	virtual GPUTimer* CreateTimer(const wchar_t* Name) = 0;
	virtual void StartTimer(GPUTimer* timer) = 0;
	virtual void StopTimer(GPUTimer* timer) = 0;
#endif


	virtual void SetPixelShader(PixelShader const* shader) = 0;
	virtual void SetVertexShader(VertexShader const* shader) = 0;
	virtual void SetComputeShader(ComputeShader const* shader) = 0;

	virtual void SetVertexLayout(VertAttDescHandle attDescHandle) = 0;

	virtual void PrepareMaterial(MaterialID mid) = 0;

	virtual ~IRenderDeviceCtx() {}

	ICBPool* CBPool = nullptr;
	IRenderDevice* Device;
//	class MaterialManager* MatManager;
};

}

