#pragma once

#include "core/Maths.h"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/transform.hpp"
#include <corecrt_math_defines.h>
#include "core/TypeInfoUtils.h"
#include "Rotator.h"

//const float MAX_ROT;

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
	DECLARE_STI_NOBASE(TTransform);

	TTransform() = default;
	TTransform(vec3 transl, vec3 scale = {1,1,1}, TRot rot = TRot{})
		: translation(transl), scale(scale), rotation(rot)
	{
	}

	template<typename TOthRot>
	TTransform(TTransform<TOthRot> const& other)
		: translation(other.translation), scale(other.scale), rotation(other.rotation)
	{
	}

	glm::vec3 GetTranslation() const { return translation; }
	pos3 GetPosition() const { return translation; }
	void	  SetTranslation(glm::vec3 val) { translation = val; }
	glm::vec3 GetScale() const { return scale; }
	void	  SetScale(glm::vec3 val) { scale = val; }
	glm::quat GetRotation() const { return rotation; }
	void	  SetRotation(glm::quat val) { rotation = val; }

	operator mat4() const {
		mat4 rotmat = ToMat4(rotation);
		mat4 result {rotmat * mat4{diagonal3x3(scale)}};
		result[3] = vec4(translation, 1);
//		result = glm::translate(result, translation);// * result;
//		result = glm::translate(translation) * result;
		return result;
	}

	mat4 const& CacheMat()
	{
		return (matCache = *this);
	}

	mat4 GetMatrix() 
	{
		return matCache;
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
	mat4 matCache;

	//mat3 matrix;
	//bool dirty = true;
};

DECLARE_CLASS_TYPEINFO(TTransform<quat>);
DECLARE_CLASS_TYPEINFO(TTransform<Rotator>);

using Transform = TTransform<quat>;
using QuatTransform = TTransform<quat>;
using RotTransform = TTransform<Rotator>;
using WorldTransform = TTransform<quat>;


inline QuatTransform operator*(QuatTransform const& first, QuatTransform const& other)
{
	return QuatTransform(first.translation + first.rotation * other.translation, first.scale * other.scale, first.rotation * other.rotation);
}

