#pragma once
#include "maths.h"
#include "TypeInfo.h"
#include "Transform.h"

class SceneComponent;

struct DirLight
{
	DECLARE_STI_NOBASE(DirLight);
	DirLight() {}
	DirLight(const vec3& dir,float intensity = 1.f)
		: dir(glm::normalize(dir)), colour(intensity) {}

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
	col3 colour;
	SceneComponent* comp;

	using Ref = u32;

	constexpr static bool const HAS_DIRECTION = true;
	constexpr static bool const HAS_POSITION = false;

	inline static String const GADGET = "dirlight";
	inline static RotTransform const GADGET_TRANS = {  };
	
};

struct PointLight
{
	DECLARE_STI_NOBASE(PointLight);
	PointLight(const vec3& pos = vec3(0), const col3& inten = { 1.f, 1.f, 1.f }, float rad = 0.5f, float fall = 1.f)
		: pos(pos), colour(inten), radius(rad), falloff(fall) {}
	vec3 pos;

	void SetPosition(vec3 const& position)
	{
		pos = position;
	}

	float radius;
	col3 colour;
	float falloff;
	SceneComponent* comp;

	constexpr static bool const HAS_DIRECTION = false;
	constexpr static bool const HAS_POSITION = true;

	inline static String const GADGET = "pointlight";
	inline static RotTransform const GADGET_TRANS = {  };

	using Ref = u32;
};

struct SpotLight
{
	DECLARE_STI_NOBASE(SpotLight);

	SpotLight(vec3 const& pos = vec3(0), vec3 const& dir = vec3{ 0, 1, 0 }, col3 const& colour = col3(1), float tangle = 1.f, float falloff = 1.f)
		: pos(pos), dir(dir), colour(colour), tangle(tangle), falloff(falloff)
	{}
	vec3 pos, dir;
	float tangle;
	float falloff;
	col3 colour;
	SceneComponent* comp;

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

DECLARE_CLASS_TYPEINFO(SpotLight);
DECLARE_CLASS_TYPEINFO(PointLight);
DECLARE_CLASS_TYPEINFO(DirLight);
