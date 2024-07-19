#pragma once
#include "core/Maths.h"
#include "core/Utils.h"

class AABB
{
public:
	AABB() = default;

	AABB(pos3 origin, vec3 radius = vec3(0))
		: Min(origin - radius), Max(origin + radius)
	{
		RASSERT((radius >= 0));
	}

	void AddPoint(pos3 point)
	{
		Min = maths::Min(point, Min);
		Max = maths::Max(point, Max);
	}

	pos3 GetOrigin()
	{
		return (Max + Min) * 0.5f;
	}

	pos3 GetExtent()
	{
		return (Max - Min);
	}

	pos3 GetRadius()
	{
		return (Max - Min) * 0.5f;
	}

	pos3 Min = pos3::Max();
	pos3 Max = pos3::Min();
};
