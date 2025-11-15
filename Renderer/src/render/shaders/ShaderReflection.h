#pragma once
#include "core/CoreTypes.h"
#include "core/Types.h"
#include "container/Vector.h"
#include "render/DeviceResource.h"

namespace rnd
{
class IShaderReflector
{
public:
	struct CBDesc
	{
		String Name;
		u32 Size;
	};

	struct BindingDesc
	{
		char const* Name = nullptr; // pointer should be valid for lifetime of reflector
		EResourceType Type = EResourceType::Unknown;
	};
	using UAVDesc = BindingDesc;
	using SRVDesc = BindingDesc;

	virtual ~IShaderReflector() {}

	virtual SmallVector<CBDesc, 4> GetConstantBuffers() = 0;
	virtual void GetBindingInfo(_Out_ SmallVector<SRVDesc, 4>& srvs, _Out_ SmallVector<UAVDesc, 4>& uavs) = 0;
};
}
