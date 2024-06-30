#pragma once
#include "core/Maths.h"
#include "core/TypeInfoUtils.h"
#include "core/Transform.h"

class SceneComponent;

enum class ELightType
{
	START = 0,
	DIR_LIGHT = START,
	POINT_LIGHT,
	SPOTLIGHT,
	COUNT
};

ITER_ENUM(ELightType)


enum class EShadingLayer : u8
{
	BASE = 0,
	DIRLIGHT,
	LIGHTS_START = DIRLIGHT,
	POINTLIGHT,
	SPOTLIGHT,
	SHADING_COUNT,
	NONE = SHADING_COUNT,
	DEPTH = NONE,
	COUNT
};

inline EShadingLayer GetLightLayer(ELightType lightType)
{
	return EnumCast<EShadingLayer>(Denum(lightType) + Denum(EShadingLayer::LIGHTS_START));
}

inline ELightType GetLightFromLayer(EShadingLayer layer)
{
	RASSERT(layer < EShadingLayer::SHADING_COUNT && layer >= EShadingLayer::LIGHTS_START);
	return EnumCast<ELightType>(Denum(layer) - Denum(EShadingLayer::LIGHTS_START));
}


struct LightData
{
	ELightType GetLightType() const { return mType; }
	const SceneComponent* GetSceneComponent() const { return comp; }
	const SceneComponent* comp;

protected:
	LightData(ELightType type)
		: mType(type) {}
	ELightType mType;
};

struct DirLight : public LightData
{
	DECLARE_STI_NOBASE(DirLight);
	DirLight()
		:LightData(ELightType::DIR_LIGHT), colour(.5f) {}
	DirLight(const vec3& dir,float intensity = 1.f)
		:LightData(ELightType::DIR_LIGHT), dir(glm::normalize(dir)), colour(intensity) {}

	 operator vec3() {
		 return dir;
	 }

	void SetDirection(vec3 direct)
	{
		dir = direct;
	}

	constexpr static vec3 GetDefaultDir()
	{
		return vec3 {0,0,1};
	}

	static RotTransform GetDefaultTrans()
	{
		return RotTransform(vec3(0, 0, 0), vec3(1, 1, 1), Rotator{45.f, -45.f, 0});
	}

	vec3 dir;
	col3 colour;

	using Ref = u32;

	constexpr static bool const HAS_DIRECTION = true;
	constexpr static bool const HAS_POSITION = false;

	inline static String const GADGET = "meshes/dirlight";
	inline static RotTransform const GADGET_TRANS = {  };
	
};

struct PointLight : public LightData
{
	DECLARE_STI_NOBASE(PointLight);
	PointLight(const vec3& pos = vec3(0), const col3& inten = { 1.f, 1.f, 1.f }, float rad = 0.5f, float fall = 1.f)
		:LightData(ELightType::POINT_LIGHT), pos(pos), colour(inten), radius(rad), falloff(fall) {}
	vec3 pos;

	void SetPosition(vec3 const& position)
	{
		pos = position;
	}

	float radius;
	col3 colour;
	float falloff;

	constexpr static bool const HAS_DIRECTION = false;
	constexpr static bool const HAS_POSITION = true;

	inline static String const GADGET = "meshes/pointlight";
	inline static RotTransform const GADGET_TRANS = { vec3(0), vec3(0.2f) };

	using Ref = u32;
};

struct SpotLight : public LightData
{
	DECLARE_STI_NOBASE(SpotLight);

	SpotLight(vec3 const& pos = vec3(0), vec3 const& dir = vec3{ 0, 1, 0 }, col3 const& colour = col3(1), float tangle = 1.f, float falloff = 1.f)
		:LightData(ELightType::SPOTLIGHT), pos(pos), dir(dir), colour(colour), tangle(tangle), falloff(falloff)
	{}
	vec3 pos, dir;
	float tangle;
	float falloff;
	col3 colour;

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

	static RotTransform GetDefaultTrans()
	{
		return RotTransform(vec3(0, -1, 0), vec3(1, 1, 1), Rotator{0, 0, 0});
	}

	using Ref = u32;

	constexpr static bool const HAS_DIRECTION = true;
	constexpr static bool const HAS_POSITION = true;

	inline static String const GADGET = "meshes/spotlight";
	inline static RotTransform const GADGET_TRANS = { vec3(0), vec3(1), Rotator {90.f,0,0} };
};

DECLARE_CLASS_TYPEINFO(SpotLight);
DECLARE_CLASS_TYPEINFO(PointLight);
DECLARE_CLASS_TYPEINFO(DirLight);
