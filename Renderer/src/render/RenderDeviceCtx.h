#pragma once
#include "render/Renderer.h"
#include <core/Utils.h>
#include "DeviceTexture.h"

enum class EBlendState : u32
{
	COL_OVERWRITE = 0x1,
	COL_ADD = 0x2,
	COL_BLEND_ALPHA = 0x4,
	COL_OBSCURE_ALPHA = 0x8,
	ALPHA_OVERWRITE = 0x10,
	ALPHA_ADD = 0x20,
	ALPHA_UNTOUCHED = 0x40,
	ALPHA_MAX
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

class IRenderDeviceCtx
{
public:
	virtual void SetRTAndDS(IRenderTarget::Ref rt, IDepthStencil::Ref ds, int RTArrayIdx = -1, int DSArrayIdx = -1) = 0;
	virtual void SetViewport(float width, float height, float TopLeftX = 0, float TopLeftY = 0) = 0;
	virtual void SetDepthMode(EDepthMode mode) = 0;
	virtual void SetBlendMode(EBlendState mode) = 0;
	virtual void ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil = 0) = 0;
};

