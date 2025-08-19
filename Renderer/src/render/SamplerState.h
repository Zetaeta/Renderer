#pragma once
#include "core/Maths.h"
#include <array>
#include "core/Utils.h"
#include "core/memory/InplaceObject.h"
#include "OpaqueHandle.h"

namespace rnd
{

enum class EComparisonFunc : u8
{
	// D3D11_COMPARION_FUNC
	None = 0,
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always
};

enum class ETextureFilter : u8
{
	Point,
	Bilinear,
	Trilinear,
	Comparison = 0x10,
	Minimum = 0x20,
	Maximum = 0x30
};

enum class ETextureAddressMode : u8
{
	// = D3D11_TEXTURE_ADDRESS_MODE
	Wrap = 1,
	Mirror,
	Clamp,
	Border,
	MirrorOnce
};

FLAG_ENUM(ETextureFilter);

struct SamplerDesc
{
	ETextureFilter Filter = ETextureFilter::Point;
	std::array<ETextureAddressMode, 3> AddressModes = { ETextureAddressMode::Wrap, ETextureAddressMode::Wrap, ETextureAddressMode::Wrap };
	float MipBias = 0.f;
	u8 MinMip = 0;
	u8 MaxMip = 255;
	float4 BorderColour{ 0 };
	EComparisonFunc Comparison = EComparisonFunc::Never;

	void SetAddressMode(ETextureAddressMode mode)
	{
		AddressModes = { mode, mode, mode };
	}

	POD_EQUALITY(SamplerDesc)
};

struct SamplerHandle : public SimpleOpaqueHandle<>
{
	SamplerHandle() = default;
	template<typename T>
		requires std::is_trivially_copyable_v<T>
	SamplerHandle(T value)
	{
		mPrivateData.As<T>() = value;
	}
};

}

