#pragma once
#include "render/Renderer.h"
#include <core/Utils.h>
#include "DeviceTexture.h"

struct MeshPart;
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

enum class EDepthMode : u32
{
	DISABLED,
	LESS,
	LESS_EQUAL,
	EQUAL,
	COUNT
};

enum class EDSClearMode : u8
{
	DEPTH = 0x1,
	STENCIL = 0x2,
	DEPTH_STENCIL = 0x3
};

enum ETextureSlot : u8
{
	E_TS_DIFFUSE = 0,
	E_TS_NORMAL,
	E_TS_ROUGHNESS,
	E_TS_EMISSIVE,
	E_TS_SHADOWMAP,
	E_TS_ENVIRONMENT,
	E_TS_COUNT
};

using TextureSlotFlags = u64;

constexpr TextureSlotFlags 
	F_TS_DIFFUSE = 1 << E_TS_DIFFUSE,
	F_TS_NORMAL = 1 << E_TS_NORMAL,
	F_TS_ROUGHNESS = 1 << E_TS_ROUGHNESS,
	F_TS_EMISSIVE = 1 << E_TS_EMISSIVE,
	F_TS_SHADOWMAP = 1 << E_TS_SHADOWMAP,
	F_TS_ENVIRONMENT = 1 << E_TS_ENVIRONMENT;

class IRenderTextureManager
{
public:
	virtual void SetTexture(ETextureSlot slot, IDeviceTexture::Ref texture) = 0;

	TextureSlotFlags& Flags() { return m_Flags; }
	void SetFlags(TextureSlotFlags flags) { m_Flags |= flags; }
	void UnsetFlags(TextureSlotFlags flags) { m_Flags &= ~flags; }
	void ClearFlags(TextureSlotFlags mask = 0) { m_Flags &= mask; }
protected:
	u64 m_Flags = 0;
};

enum class EShaderType : u8
{
	Vertex,
	Pixel,
	Count
};

class IRenderDeviceCtx
{
public:
	virtual void SetRTAndDS(IRenderTarget::Ref rt, IDepthStencil::Ref ds, int RTArrayIdx = -1, int DSArrayIdx = -1) = 0;
	virtual void SetViewport(float width, float height, float TopLeftX = 0, float TopLeftY = 0) = 0;
	inline void SetViewport(u32 width, u32 height, u32 TopLeftX = 0, u32 TopLeftY = 0)
	{
		SetViewport(float(width), float(height), float(TopLeftX), float(TopLeftY));
	}
	virtual void SetDepthMode(EDepthMode mode) = 0;
	virtual void SetBlendMode(EBlendState mode) = 0;
	virtual void ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil = 0) = 0;
	virtual void ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour) = 0;
	virtual void DrawCubemap(IDeviceTextureCube* cubemap) = 0;
	virtual void DrawMesh(MeshPart const& meshPart, EShadingLayer layer, bool useMaterial = true) = 0;
	virtual IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size = 0) = 0; 
	virtual void			 SetConstantBuffers(EShaderType shaderType, IConstantBuffer** buffers, u32 numBuffers) = 0;

	IRenderDevice* Device;
	IRenderTextureManager* TextureManager;
	class MaterialManager* MatManager;
};

}

