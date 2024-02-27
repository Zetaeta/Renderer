#pragma once
#include "TypeInfo.h"
#include "maths.h"

struct Rotator
{
	DECLARE_STI_NOBASE(Rotator);

	float		  pitch = 0;
	float		  yaw = 0;
	float		  roll = 0;
	inline float& operator[](u32 i)
	{
		RASSERT(i < 3);
		return (&pitch)[i];
	}

	constexpr static float ToRad(float f)
	{
		return f * M_PI / 180.f;
	}

	inline operator quat() const
	{
		const vec3 X = { 1.f, 0, 0 };
		const vec3 Y = { 0, 1.f, 0 };
		const vec3 Z = { 0, 0, 1.f };
		return angleAxis(ToRad(roll), Z) * angleAxis(ToRad(pitch), X) * angleAxis(ToRad(yaw), Y);
	}

	void Normalize()
	{
	}
};

//DECLARE_CLASS_TYPEINFO(Rotator);

DECLARE_AGGREGATE_CONTAINER_TYPEINFO(Rotator, float);
DECLARE_AGGREGATE_CONTAINER_TYPEINFO(quat, float);