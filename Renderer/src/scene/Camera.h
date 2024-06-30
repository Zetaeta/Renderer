#pragma once

#include <core/Maths.h>
#include <core/Types.h>
#include <core/Utils.h>
#include <core/Transform.h>

enum class ECameraType : u8
{
	ORTHOGRAPHIC,
	PERSPECTIVE,
	CUBE
};

class Camera
{

public:
	Camera(ECameraType type, pos3 const& position = pos3{0})
		: CameraType(type), Position(position) {}

	Camera(ECameraType type, QuatTransform const& trans);

	vec3 const& GetPosition() const
	{
		return Position;
	}

	mat4 const& WorldToCamera() const
	{
		if (Dirty)
		{
			Recompute();
		}

		return W2C;
	}

	mat4 GetProjWorld() const
	{
		if (Dirty)
		{
			Recompute();
		}
		return Projection * W2C;
	}

	mat4 GetInverseProjWorld() const
	{
		if (Dirty)
		{
			Recompute();
		}
		return glm::inverse(GetProjWorld());
	}

	void SetPosition(vec3 pos)
	{
		Position = pos;
		Dirty = true;
	}

	void SetRotation(mat3 rotMat)
	{
		Rotation = rotMat;
		Dirty = true;
	}

	void SetTransform(QuatTransform const& trans);

	void SetFOVs(float horizontal, float vertical)
	{
		RASSERT(CameraType == ECameraType::PERSPECTIVE, "Setting FOV for non-perspective camera");
		HorizExtent = tan(horizontal);
		VertiExtent = tan(vertical);
		Dirty = true;
	}

	void SetHFOVAndAspect(float horizontalFOV, float aspectRatio)
	{
		RASSERT(CameraType == ECameraType::PERSPECTIVE, "Setting FOV for non-perspective camera");
		HorizExtent = tan(horizontalFOV);
		VertiExtent = HorizExtent / aspectRatio;
		Dirty = true;
	}

	void SetViewExtent(float horizontal, float vertical)
	{
	//	RASSERT(CameraType == ECameraType::ORTHOGRAPHIC, "Setting view extent for non-orthographic camera");
		Dirty = Dirty || (horizontal != HorizExtent) || (vertical != VertiExtent);
		HorizExtent = horizontal;
		VertiExtent = vertical;
	}

	mat3 const& GetView() const
	{
		if (Dirty)
		{
			Recompute();
		}
		return ViewRotation;
	}
	 
	void SetNearClip(float distance)
	{
		Dirty = Dirty || (NearClip != distance);
		NearClip = distance;
	}

	void SetFarClip(float distance)
	{
		Dirty = Dirty || (FarClip != distance);
		FarClip = distance;
	}

	void Recompute() const;

	mat4 const& GetProjection() const
	{
		if (Dirty)
		{
			Recompute();
		}

		return Projection;
	}

	Vector<mat4> const& GetCubeProjections() const
	{
		RASSERT(CameraType == ECameraType::CUBE);
		if (Dirty)
		{
			Recompute();
		}
		return CubeProjs;
	}

	ECameraType GetCameraType() const { return CameraType; }



	using Ref = Camera*;
	using ConstRef = Camera const*;

protected:
	vec3  Position{ 0 };
	mat3  Rotation = glm::identity<mat3>();
	mutable mat3  ViewRotation = glm::identity<mat3>();

	ECameraType CameraType = ECameraType::PERSPECTIVE;
	// length units for orthographic, tan for perspective
	float HorizExtent = 1;
	float VertiExtent = 1;
	float NearClip = 0.1f;
	float FarClip = 1000;

	mutable bool Dirty = true;
	mutable mat4 W2C;

	mutable mat4		 Projection;
	mutable Vector<mat4> CubeProjs;
};
