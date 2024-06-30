#pragma once
#include "core/Maths.h"
#include "common/Input.h"
#include "core/Utils.h"
#include "Camera.h"

class UserCamera : public Camera
{
public:
	UserCamera(Input* input)
	:Camera(ECameraType::PERSPECTIVE), m_Input(input){}

	void Tick(float deltaTime);

	bool ViewChanged() { return m_ViewChanged; }
	void CleanView() {m_ViewChanged = false;}


	float movespeed = .005f;
	float turnspeed = .001f;
	private:
	vec2 m_LastMouse;
	bool m_ViewChanged;
	Input* m_Input;
};
