#pragma once
#include <memory>
#include <core/Maths.h>
#include <core/Utils.h>

enum class ETextureDimension : u8
{
	TEX_2D,
	TEX_CUBE
};

struct DeviceResourceDesc
{
	String DebugName;
};

struct DepthStencilDesc : DeviceResourceDesc
{
	ETextureDimension Dimension;
	u32 Width = 0;
	u32 Height = 0;
};

struct RenderTargetDesc : DeviceResourceDesc
{
	ETextureDimension Dimension;
	u32 Width = 0;
	u32 Height = 0;
};

#define DEFINE_DEVICE_RESOURCE_GETTER(FunctionName)\
	virtual void* FunctionName() const = 0;\
	template<typename T>\
	T* FunctionName() { return static_cast<T*>(FunctionName()); }


class IDepthStencil
{
public:
	IDepthStencil(DepthStencilDesc desc)
		: Desc(desc) {}

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

	using Ref = std::shared_ptr<IRenderTarget>;

	RenderTargetDesc Desc;
};

enum ETextureFlags
{
	TF_NONE = 0,
	TF_DEPTH = 0x1,
	TF_SRGB = 0x2,
	TF_RENDER_TARGET = 0x3
};

struct DeviceTextureDesc : DeviceResourceDesc
{
	ETextureFlags flags;
	u32 width;
	u32 height;
	u32 numMips = 1;
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


class IDeviceTexture
{
public:
	IDeviceTexture(DeviceTextureDesc const& desc)
		: Desc(desc) {}
	using Ref = std::shared_ptr<IDeviceTexture>;

	virtual void* GetTextureHandle() const = 0;
	template<typename T>
	T* GetTextureHandle() { return static_cast<T*>(GetTextureHandle()); }

	bool IsDepthStencil() const { return Desc.flags & TF_DEPTH; }


	virtual IRenderTarget::Ref GetRT() = 0;
	virtual IDepthStencil::Ref GetDS() = 0;

	DeviceTextureDesc Desc;
};

using IDeviceTextureCube = IDeviceTexture;

using DeviceTextureRef = std::shared_ptr<IDeviceTexture>;
//constexpr DeviceTextureRef INVALID_DEV_TEX = nullptr;
#define INVALID_DEV_TEX nullptr
