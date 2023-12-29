#pragma once

#include "maths.h"

struct Rotator
{
	float yaw;
	float pitch;
	float roll;

};

struct Transform
{
	Transform() = default;
	Transform(vec3 transl, vec3 scale = {1,1,1}, quat rot = quat{})
		: translation(transl), scale(scale), rotation(rot)
	{
	}

	glm::vec3 GetTranslation() const { return translation; }
	void	  SetTranslation(glm::vec3 val) { translation = val; }
	glm::vec3 GetScale() const { return scale; }
	void	  SetScale(glm::vec3 val) { scale = val; }
	glm::quat GetRotation() const { return rotation; }
	void	  SetRotation(glm::quat val) { rotation = val; }

	operator mat4() const {
		mat3 rotmat = mat3_cast(rotation);
		mat4 result(rotmat * diagonal3x3(scale));
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
		return rotation * (vec * scale) + translation;
	}

public:
	vec3 translation = vec3{ 0.f };
	vec3 scale = vec3{ 1.f };
	quat rotation;

	//mat3 matrix;
	//bool dirty = true;
};


