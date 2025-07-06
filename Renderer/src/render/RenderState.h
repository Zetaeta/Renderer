#pragma once
#include "core/CoreTypes.h"
#include "core/Utils.h"
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


}
