#pragma once
#include "core/Types.h"

namespace rnd
{
class IDeviceSurface
{
public:
	virtual void Present() = 0;

	virtual void RequestResize(uvec2 newSize) = 0;
};
}
