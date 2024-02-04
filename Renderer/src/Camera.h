#pragma once
#include "maths.h"
#include "Input.h"

class Camera
{
public:
	Camera(Input* input)
	:m_Input(input){}

	void Tick(float deltaTime);

	bool ViewChanged() { return m_ViewChanged; }
	void CleanView() {m_ViewChanged = false;}

	mat4 WorldToCamera() {
		mat4 m(view);
		m[3] = vec4(position, 1.f);
		return glm::inverse(m);
	}

	vec3 position{0};
	mat3 rotation = glm::identity<mat3>();
	mat3 view = glm::identity<mat3>();
	mat3& inverseView = rotation;
	float movespeed = .005f;
	float turnspeed = .001f;
	private:
	vec2 m_LastMouse;
	bool m_ViewChanged;
	Input* m_Input;
};
