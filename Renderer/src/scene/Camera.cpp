#include "Camera.h"
#include "glm/gtx/transform.hpp"



Camera::Camera(ECameraType type, QuatTransform const& trans)
	: Camera(type, trans.GetPosition())
{
	SetRotation(mat3(trans.GetRotation()));
}

void Camera::SetTransform(QuatTransform const& trans)
{
	Position = trans.GetPosition();
	Rotation = mat3(trans.GetRotation());
	Dirty = true;
}

void Camera::Recompute() const
{
	ViewRotation = glm::inverse(Rotation);
	if (CameraType == ECameraType::ORTHOGRAPHIC)
	{
		Projection = glm::transpose(mat4{
			{ 1.f / HorizExtent, 0, 0, 0 },
			{ 0, 1.f / VertiExtent, 0, 0 },
			{ 0, 0, 1 / (FarClip - NearClip), -NearClip / (FarClip - NearClip) },
			{ 0, 0, 0, 1 },
		});
	}
	else
	{
		Projection = glm::transpose(mat4{
			{ 1.f / HorizExtent, 0, 0, 0 },
			{ 0, 1.f / VertiExtent, 0, 0 },
			{ 0, 0, (FarClip) / (FarClip - NearClip), -FarClip * NearClip / (FarClip - NearClip) },
			{ 0, 0, 1, 0 },
		});
	}

	if (CameraType == ECameraType::CUBE)
	{
		CubeProjs.resize(6);
		constexpr float ROTVAL = float(M_PI_2);
		static mat4		rots[] = {
			glm::rotate(ROTVAL, vec3{ 0, -1, 0 }),		// +Z -> +X
			glm::rotate(ROTVAL, vec3{ 0, 1, 0 }),		// +Z -> -X
			glm::rotate(ROTVAL, vec3{ 1, 0, 0 }),		// +Z -> +Y
			glm::rotate(ROTVAL, vec3{ -1, 0, 0 }),		// +Z -> +Y
			glm::identity<mat4>(),						// +Z -> +Z
			glm::rotate(ROTVAL * 2.f, vec3{ 0, 1, 0 }), // +Z -> -Z
		};

		for (int i = 0; i < 6; ++i)
		{
			CubeProjs[i] = Projection * rots[i];
		}
	}

	mat4 m(Rotation);
	m[3] = vec4(Position, 1.f);
	W2C = glm::inverse(m);
	Dirty = false;
}
