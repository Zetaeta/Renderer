#pragma once

#include <core/Maths.h>
#include <core/Utils.h>
#include "DeviceResource.h"

#include <memory>
#include "common/CommonEnums.h"

class Texture;


namespace rnd
{

u32 GetPixelSize(ETextureFormat format);
u32 GetNumChannels(ETextureFormat format);

enum class ETextureDimension : u8
{
	TEX_2D,
	TEX_CUBE
};

struct DepthStencilDesc : DeviceChildDesc
{
	ETextureDimension Dimension = ETextureDimension::TEX_2D;
	ETextureFormat Format = ETextureFormat::D24_Unorm_S8_Uint;
	u32 Width = 0;
	u32 Height = 0;
};

struct RenderTargetDesc : DeviceChildDesc
{
	ETextureDimension Dimension = ETextureDimension::TEX_2D;
	ETextureFormat Format = ETextureFormat::RGBA8_Unorm_SRGB;
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
//	using Ref = std::shared_ptr<IDepthStencil>;

	struct Ref
	{
		Ref(std::nullptr_t n) {}

		Ref(std::shared_ptr<IDepthStencil> const& rt = nullptr, u32 idx = 0)
		:DS(rt), Index(idx) { }
		//operator std::shared_ptr<IDepthStencil>() const {
		//	return DS;
		//}
		operator bool() const
		{
			return DS != nullptr;
		}
		bool operator!=(std::nullptr_t) { return DS != nullptr; }
		IDepthStencil* operator->() const {return DS.get(); }
		IDepthStencil* get() const {return DS.get(); }
		Ref& operator=(std::shared_ptr<IDepthStencil> const& ds)
		{
			DS = ds;
			Index = 0;
			return *this;
		}

		std::shared_ptr<IDepthStencil> DS;
		u32 Index = 0;
	};
	DepthStencilDesc const& GetDesc() const { return Desc; }

	DepthStencilDesc Desc;
};

class IRenderTarget
{
public:
	IRenderTarget(RenderTargetDesc const& desc)
		: Desc(desc) {}
	virtual ~IRenderTarget() {}

	RenderTargetDesc const& GetDesc() { return Desc; }
	virtual OpaqueData<8> GetRTData() = 0;
//	virtual void*	 GetDSData() = 0;
	DEFINE_DEVICE_RESOURCE_GETTER(GetData);

	struct Ref
	{
		Ref(std::nullptr_t n) {}
		Ref(std::shared_ptr<IRenderTarget> rt = nullptr, u32 idx = 0)
		:RT(rt), Index(idx) { }

		operator bool() const
		{
			return RT != nullptr;
		}
		IRenderTarget* operator->() const {return RT.get(); }
		IRenderTarget* get() const {return RT.get(); }
		Ref& operator=(std::shared_ptr<IRenderTarget> const& rt)
		{
			RT = rt;
			Index = 0;
			return *this;
		}
		bool operator!=(std::nullptr_t) { return RT != nullptr; }
		std::shared_ptr<IRenderTarget> RT;
		u32 Index = 0;
	};

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


struct DeviceTextureDesc : public DeviceResourceDesc
{
	DeviceTextureDesc()
	{
		ResourceType = EResourceType::Texture2D;
	}

	ETextureFlags Flags = TF_SRV;
	ETextureFormat Format = ETextureFormat::RGBA8_Unorm;
	u32 Width;
	u32 Height;
	u16 ArraySize = 1;
	u8 NumMips = 1;
	u8 SampleCount = 1;

	u64 GetSize() const;
	u32 GetRowPitch() const;

	uvec2 Extent() const
	{
		return {Width, Height};
	}
};

enum class ECubemapDataFormat : u8
{
	FoldUp = 0
};

struct CubemapData
{
	ECubemapDataFormat Format;
	void* Data = nullptr;
	std::shared_ptr<Texture> Tex;
};

using TextureData = void const*;

constexpr ShaderResourceId SRV_StencilBuffer = 1ull << 63;
constexpr ShaderResourceId SRV_Texture = 0;

class IDeviceTexture : public IDeviceResource
{
public:
	IDeviceTexture(IRenderDevice* device, DeviceTextureDesc const& desc)
		:IDeviceResource(device), Desc(desc) {}

	virtual ~IDeviceTexture() {}
	using Ref = std::shared_ptr<IDeviceTexture>;

	virtual OpaqueData<8> GetTextureHandle() const = 0;
	template<typename T>
	T GetTextureHandle() { return GetTextureHandle().As<T>(); }

	bool IsDepthStencil() const { return Desc.Flags & TF_DEPTH; }

	DEFINE_DEVICE_RESOURCE_GETTER(GetData);

	virtual IRenderTarget::Ref GetRT() = 0;
	virtual IDepthStencil::Ref GetDS() = 0;

	virtual void CreateSRV(u32 srvIndex) = 0;

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

//size_t GetTextureSize(ETextureFormat format, u32 width, u32 height);
//size_t GetTextureRowPitch(ETextureFormat format, u32 width, u32 height);
