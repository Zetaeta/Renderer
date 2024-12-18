#pragma once

#include <core/Maths.h>
#include <core/Utils.h>
#include "DeviceResource.h"

#include <memory>
#include "common/CommonEnums.h"

namespace rnd
{

enum class ETextureDimension : u8
{
	TEX_2D,
	TEX_CUBE
};

enum class EDeviceResourceType : u8
{
	Texture1D,
	Texture2D,
	Texture3D,
	Buffer
};

struct DeviceChildDesc
{
	String DebugName;
};

struct DepthStencilDesc : DeviceChildDesc
{
	ETextureDimension Dimension;
	u32 Width = 0;
	u32 Height = 0;
};

struct RenderTargetDesc : DeviceChildDesc
{
	ETextureDimension Dimension;
	u32 Width = 0;
	u32 Height = 0;
};


class IDepthStencil
{
public:
	IDepthStencil(DepthStencilDesc desc)
		: Desc(desc) {}

	virtual ~IDepthStencil() {}

//	virtual void*	 GetData() const = 0;
	DEFINE_DEVICE_RESOURCE_GETTER(GetData);
	using Ref = std::shared_ptr<IDepthStencil>;
	DepthStencilDesc const& GetDesc() const { return Desc; }

	DepthStencilDesc Desc;
};

class IRenderTarget
{
public:
	IRenderTarget(RenderTargetDesc const& desc)
		: Desc(desc) {}
	RenderTargetDesc const& GetDesc() { return Desc; }
	virtual bool HasColour() const = 0;
	virtual bool HasDepth() const = 0;
	virtual void*	 GetRTData() = 0;
	virtual void*	 GetDSData() = 0;
	DEFINE_DEVICE_RESOURCE_GETTER(GetData);

	using Ref = std::shared_ptr<IRenderTarget>;

	RenderTargetDesc Desc;
};

enum ETextureFlags
{
	TF_NONE = 0,
	TF_DEPTH = 0x1,
	TF_SRGB = 0x2,
	TF_RenderTarget = 0x4,
	TF_CpuReadback = 0x8,
	TF_SRV = 0x10,
	TF_StencilSRV = 0x20,
	TF_UAV = 0x40,
//	TF_Stencil = 0x80,
};

FLAG_ENUM(ETextureFlags);

//constexpr ETextureFlags TF_DEPTH_STENCIL = TF_DEPTH | TF_Stencil;

struct DeviceResourceDesc : public DeviceChildDesc
{
	EDeviceResourceType ResourceType;
};


struct DeviceTextureDesc : public DeviceResourceDesc
{
	DeviceTextureDesc()
	{
		ResourceType = EDeviceResourceType::Texture2D;
	}

	ETextureFlags Flags = TF_SRV;
	ETextureFormat Format = ETextureFormat::RGBA8_Unorm;
	u32 Width;
	u32 Height;
	u16 ArraySize = 1;
	u8 NumMips = 1;
	u8 SampleCount = 1;
};

enum ECubemapDataFormat
{
	FoldUp = 0
};

struct CubemapData
{
	ECubemapDataFormat Format;
	void* Data;
};

using TextureData = void const*;

constexpr ShaderResourceId SRV_StencilBuffer = 1;
constexpr ShaderResourceId SRV_Texture = 0;

class IDeviceTexture : public IDeviceResource
{
public:
	IDeviceTexture(DeviceTextureDesc const& desc)
		: Desc(desc) {}

	virtual ~IDeviceTexture() {}
	using Ref = std::shared_ptr<IDeviceTexture>;

	virtual void* GetTextureHandle() const = 0;
	template<typename T>
	T* GetTextureHandle() { return static_cast<T*>(GetTextureHandle()); }

	bool IsDepthStencil() const { return Desc.Flags & TF_DEPTH; }

	DEFINE_DEVICE_RESOURCE_GETTER(GetData);

	virtual IRenderTarget::Ref GetRT() = 0;
	virtual IDepthStencil::Ref GetDS() = 0;

	virtual void CreateSRV() = 0;

	DeviceTextureDesc Desc;
};


struct DeviceSubresource
{
	DeviceSubresource(IDeviceTexture* tex, u16 arrayIdx = 0, u16 mipIdx = 0)
		: Resource(tex), ArrayIdx(arrayIdx), MipIdx(mipIdx)
	{
	}

	DeviceSubresource(IDeviceTexture::Ref tex, u16 arrayIdx = 0, u16 mipIdx = 0)
		: Resource(tex.get()), ArrayIdx(arrayIdx), MipIdx(mipIdx)
	{
	}
	IDeviceTexture* Resource = nullptr;
	u16 ArrayIdx = 0;
	u16 MipIdx = 0;
};

using IDeviceTextureCube = IDeviceTexture;

using DeviceTextureRef = std::shared_ptr<IDeviceTexture>;
//constexpr DeviceTextureRef INVALID_DEV_TEX = nullptr;
#define INVALID_DEV_TEX nullptr

}
