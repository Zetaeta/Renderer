#pragma once
#include "core/Types.h"

#define DXCALL(expr) HR_ERR_CHECK(expr)

#ifdef _DEBUG
#define DXPARANOID(expr) DXCALL(expr)
#else
#define DXPARANOID(expr) expr
#endif

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

void UnpackSubresource(u32 subresource, u32 arraySize, auto& outArrayIdx, auto& outMip)
{
	outArrayIdx = subresource / arraySize;
	outMip = subresource % arraySize;
}

}
