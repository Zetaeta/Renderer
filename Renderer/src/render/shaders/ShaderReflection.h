#pragma once
#include "core/CoreTypes.h"
#include "core/Types.h"
#include "container/Vector.h"

namespace rnd
{
class IShaderReflector
{
public:
	struct CBDesc
	{
		Name Name;
		u32 Size;
	};
	virtual ~IShaderReflector() {}

	virtual SmallVector<CBDesc, 4> GetConstantBuffers() = 0;
};
}
