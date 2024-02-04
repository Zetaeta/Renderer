#pragma once
#include "maths.h"

struct DirLight
{
	DirLight(const vec3& dir,float intensity = 1.f)
		: dir(glm::normalize(dir)), intensity(intensity) {}

	 operator vec3() {
		 return dir;
	 }

	void SetDirection(vec3 direct)
	{
		dir = direct;
	}

	constexpr static vec3 GetDefaultDir()
	{
		return vec3 {-1,-1,-1};
	}

	vec3 dir;
	float intensity;

	using Ref = u32;

	constexpr static bool const HAS_DIRECTION = true;
	constexpr static bool const HAS_POSITION = false;

	inline static String const GADGET = "dirlight";
	
};

struct PointLight
{
	PointLight(const vec3& pos, const col3& inten = { 1.f, 1.f, 1.f }, float rad = 0.5f, float fall = 1.f):pos(pos), intensity(inten), radius(rad), falloff(fall) {}
	vec3 pos;

	void SetPosition(vec3 const& position)
	{
		pos = position;
	}

	float radius;
	col3 intensity;
	float falloff;
	constexpr static bool const HAS_DIRECTION = false;
	constexpr static bool const HAS_POSITION = true;

	inline static String const GADGET = "pointlight";

	using Ref = u32;
};

struct SpotLight
{
	SpotLight(vec3 const& pos = vec3(0), vec3 const& dir = vec3{ 0, 1, 0 }, col3 const& colour = col3(1), float tangle = 1.f, float falloff = 1.f)
		: pos(pos), dir(dir), colour(colour), tangle(tangle), falloff(falloff)
	{}
	vec3 pos, dir;
	float tangle;
	float falloff;
	col3 colour;

	mat4 trans;

	void SetPosition(vec3 const& position)
	{
		pos = position;
	}

	void SetDirection(vec3 direct)
	{
		dir = direct;
	}

	constexpr static vec3 GetDefaultDir()
	{
		return vec3 {0,0,1};
	}

	using Ref = u32;

	constexpr static bool const HAS_DIRECTION = true;
	constexpr static bool const HAS_POSITION = true;

	inline static String const GADGET = "spotlight.glb";
	inline static RotTransform const GADGET_TRANS = { vec3(0), vec3(1), Rotator {90.f,0,0} };
};

