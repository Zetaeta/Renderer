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

	using CBDesc = ShaderCBInfo;

	virtual ~IShaderReflector() {}

	virtual SmallVector<CBDesc, 4> GetConstantBuffers() = 0;
	virtual void GetBindingInfo(_Out_ SmallVector<SRVBindingDesc, 4>& srvs, _Out_ SmallVector<UAVBindingDesc, 4>& uavs) = 0;

	virtual ShaderParamersInfo GetParamsInfo()
	{
		ShaderParamersInfo result;
		result.ConstantBuffers = GetConstantBuffers();
		GetBindingInfo(result.SRVs, result.UAVs);
		return result;
	}
};
}
