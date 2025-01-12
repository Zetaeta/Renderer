#pragma once
#include "core/Types.h"

#define DXCALL(expr) HR_ERR_CHECK(expr)

namespace rnd::dx
{

template<typename ResourcePtr>
void SetResourceName(const ResourcePtr& resource, const String& str)
{
	if (!str.empty())
	{
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, (u32) str.size(), str.c_str());
	}
}

#define NAME_D3DOBJECT(obj, name) SetResourceName(obj, name)

}
