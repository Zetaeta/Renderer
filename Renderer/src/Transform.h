#pragma once

#include "maths.h"
#include "glm/gtx/euler_angles.hpp"
#include <corecrt_math_defines.h>

//const float MAX_ROT;

struct Rotator
{
	float pitch = 0;
	float yaw = 0;
	float roll = 0;
	inline float& operator[](u32 i)
	{
		return (&pitch)[i];
	}

	constexpr static float ToRad(float f)
	{
		return f * M_PI / 180.f;
	}

	inline operator quat() const
	{
		const vec3 X = {1.f,0,0};
		const vec3 Y = {0,1.f,0};
		const vec3 Z = {0,0,1.f};
		return angleAxis (ToRad(roll), Z) * angleAxis(ToRad(pitch), X) * angleAxis(ToRad(yaw), Y);
	}

	void Normalize()
	{
		
	}
};

inline mat4 ToMat4(Rotator const& rot) {
	return eulerAngleYXZ(rot.ToRad(rot.yaw), rot.ToRad(rot.pitch), rot.ToRad(rot.roll));
}

inline mat4 ToMat4(quat const& q)
{
	return mat4_cast(q);
}

//inline vec3 operator*(quat const& q, vec3 const& v)
//{
//	return mat3_cast(q) * v;
//}

template<typename TRot>
struct TTransform
{
	TTransform() = default;
	TTransform(vec3 transl, vec3 scale = {1,1,1}, TRot rot = TRot{})
		: translation(transl), scale(scale), rotation(rot)
	{
	}

	template<typename TOthRot>
	TTransform(TTransform<TOthRot> const& other)
		: translation(other.translation), scale(other.scale), rotation(static_cast<quat>(other.rotation))
	{
	}

	glm::vec3 GetTranslation() const { return translation; }
	void	  SetTranslation(glm::vec3 val) { translation = val; }
	glm::vec3 GetScale() const { return scale; }
	void	  SetScale(glm::vec3 val) { scale = val; }
	glm::quat GetRotation() const { return rotation; }
	void	  SetRotation(glm::quat val) { rotation = val; }

	operator mat4() const {
		mat4 rotmat = ToMat4(rotation);
		mat4 result {rotmat * mat4{diagonal3x3(scale)}};
		result[3] = vec4(translation, 1);
		return result;
	}
	/* mat3 GetMatrix()
	{
		if (dirty) {
			
		}
	}
	void RecomputeMatrix() {
		//matrix = rotation.
	}*/


	vec3 operator*(const vec3& vec) const {
		return vec3(mat4(*this) * vec4(vec, 0));//rotation * (vec * scale) + translation;
	}

	pos3 operator*(const pos3& pos) const {
		return pos3(mat4(*this) * vec4(pos, 1));//rotation * (vec * scale) + translation;
	}

public:
	vec3 translation = vec3{ 0.f };
	vec3 scale = vec3{ 1.f };
	TRot rotation;

	//mat3 matrix;
	//bool dirty = true;
};


using Transform = TTransform<quat>;
using QuatTransform = TTransform<quat>;
using RotTransform = TTransform<Rotator>;

inline QuatTransform operator*(QuatTransform const& first, QuatTransform const& other)
{
	return QuatTransform(first.translation + first.rotation * other.translation, first.scale * other.scale, first.rotation * other.rotation);
}

