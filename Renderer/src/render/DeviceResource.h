#pragma once

#include "core/Maths.h"
#include <memory>
#include <span>

template<typename T>
using Span = std::span<T>;

template<typename T>
Span<T> Single(T& value)
{
	return Span<T>(&value, 1u);
}

namespace rnd
{

#define DEFINE_DEVICE_RESOURCE_GETTER(FunctionName)\
	virtual void* FunctionName() const = 0;\
	template<typename T>\
	T* FunctionName() { return static_cast<T*>(FunctionName()); }

struct MappedResource
{
	void* Data;
	u32 RowPitch;
	u32 DepthPitch;
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


class IDeviceResource
{
public:
	virtual ~IDeviceResource() {}
	virtual MappedResource Map(u32 subResource, ECpuAccessFlags flags) = 0;
	virtual void		   Unmap(u32 subResource) = 0;

	virtual void* GetShaderResource(ShaderResourceId id = 0) = 0;
	template <typename T>
	T GetShaderResource(ShaderResourceId id = 0)
	{
		return reinterpret_cast<T>(GetShaderResource(id));
	}

	virtual void* GetUAV(UavId id = {}) = 0;
	template <typename T>
	T GetUAV(UavId id = {})
	{
		return reinterpret_cast<T>(GetUAV(id));
	}
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
