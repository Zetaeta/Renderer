#pragma once

#include "core/Maths.h"
#include <memory>

namespace rnd
{

struct MappedResource
{
	void* Data;
	u32 RowPitch;
	u32 DepthPitch;
};

using ShaderResourceId = u64;

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
};

using DeviceResourceRef = std::shared_ptr<IDeviceResource>;

struct ResourceView
{
	DeviceResourceRef Resource;
	u64 ViewId = 0;

	ResourceView() {}
	ResourceView(DeviceResourceRef resource)
		: Resource(resource) {}

	ResourceView(DeviceResourceRef resource, u64 view)
		: Resource(resource), ViewId(view) {}

	template<typename T>
	T Get() const
	{
		return Resource ? Resource->GetShaderResource<T>(ViewId) : T{};
	}
};

using ShaderResources = Vector<ResourceView>;


} // namespace rnd
