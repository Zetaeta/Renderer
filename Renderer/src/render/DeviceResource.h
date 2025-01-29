#pragma once

#include "core/Maths.h"
#include <memory>
#include <span>
#include "core/memory/CopyableMemory.h"


template<typename T>
Span<T> Single(T& value)
{
	return Span<T>(&value, 1u);
}

namespace rnd
{

#define DEFINE_DEVICE_RESOURCE_GETTER(FunctionName)\
	virtual OpaqueData<8> FunctionName() const = 0;\
	template<typename T>\
	T FunctionName() { return FunctionName().As<T>(); }

struct MappedResource
{
	void* Data;
	u32 RowPitch;
	u32 DepthPitch;
	u32 Handle;
	std::function<void()> Release;
};

using ShaderResourceId = u64;
using UavId = u64;

enum class ECpuAccessFlags
{
	None,
	Read = 0x1,
	Write = 0x2
};

FLAG_ENUM(ECpuAccessFlags);

//enum class EResourceType : u8
//{
//	Unknown		 = 0,
//	Texture		 = 0x01,
//	Buffer		 = 0x10,
//	VertexBuffer = 0x11
//};
//

enum class EResourceType : u8
{
	Unknown = 0,
	Texture = 0x1,
	Texture1D = 0x3,
	Texture2D = 0x5,
	TextureCube = 0x7,
	Texture3D = 0x9,
	Buffer = 0x10,
	VertexBuffer = 0x20
};

struct DeviceChildDesc
{
	String DebugName;
};

struct DeviceResourceDesc : public DeviceChildDesc
{
	EResourceType ResourceType;
};

class IDeviceResource
{
public:
	virtual ~IDeviceResource() {}
	virtual MappedResource Map(u32 subResource, ECpuAccessFlags flags) = 0;
	virtual void		   Unmap(u32 subResource) = 0;

	EResourceType GetResourceType();

	virtual CopyableMemory<8> GetShaderResource(ShaderResourceId id = 0) = 0;
	template <typename T>
	T GetShaderResource(ShaderResourceId id = 0)
	{
		return GetShaderResource(id).As<T>();
	}

	// eg ID3D12Resource*, ID3D11Texture*
	virtual OpaqueData<8> GetRHIResource() const = 0;

	virtual OpaqueData<8> GetUAV(UavId id = {}) = 0;
	template <typename T>
	T GetUAV(UavId id = {})
	{
		return GetUAV(id).As<T>();
	}

	virtual UavId CreateUAV(u32 subresourceIdx) { return 0; }
};

using DeviceResourceRef = std::shared_ptr<IDeviceResource>;

struct ResourceView
{
	DeviceResourceRef Resource;
	u64 ViewId = 0;

	ResourceView(DeviceResourceRef resource = nullptr, u64 view = 0)
		: Resource(resource), ViewId(view) {}

	template<typename T>
	T Get() const
	{
		return Resource ? Resource->GetShaderResource<T>(ViewId) : T{};
	}

};

struct UnorderedAccessView
{
	DeviceResourceRef Resource;
	UavId ViewId = 0;

	UnorderedAccessView(DeviceResourceRef resource = nullptr, UavId view = {})
		: Resource(resource), ViewId(view) {}

	template<typename T>
	T Get() const
	{
		return Resource ? Resource->GetUAV<T>(ViewId) : T{};
	}
};


using ShaderResources = Span<ResourceView const>;
using UnorderedAccessViews = Span<UnorderedAccessView const>;


} // namespace rnd
