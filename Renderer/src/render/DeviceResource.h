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
	Texture3D = 0x9,
	Buffer = 0x10,
	VertexBuffer = 0x20
};

class IDeviceResource
{
public:
	virtual ~IDeviceResource() {}
	virtual MappedResource Map(u32 subResource, ECpuAccessFlags flags) = 0;
	virtual void		   Unmap(u32 subResource) = 0;

	virtual CopyableMemory<8> GetShaderResource(ShaderResourceId id = 0) = 0;
	template <typename T>
	T GetShaderResource(ShaderResourceId id = 0)
	{
		return GetShaderResource(id).As<T>();
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
