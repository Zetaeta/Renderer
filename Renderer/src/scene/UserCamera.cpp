#include "scene/UserCamera.h"
#include <glm/gtx/euler_angles.hpp>
#include <iostream>


void UserCamera::Tick(float deltaTime)
{
	vec2 mousePos = m_Input->GetMousePosition();
	//std::cout << mousePos.x << ", " << mousePos.y << std::endl;
	vec2 mouseMove = (mousePos - m_LastMouse) * deltaTime * turnspeed;
	vec3 cameraMove{0};

	if (m_Input->IsKeyDown(Input::Key::W)) {
		cameraMove.z += deltaTime;
	}
	if (m_Input->IsKeyDown(Input::Key::S)) {
		cameraMove.z -= deltaTime;
	}
	if (m_Input->IsKeyDown(Input::Key::D)) {
		cameraMove.x += deltaTime;
	}
	if (m_Input->IsKeyDown(Input::Key::A)) {
		cameraMove.x -= deltaTime;
	}
	if (m_Input->IsKeyDown(Input::Key::Q)) {
		cameraMove.y += deltaTime;
	}
	if (m_Input->IsKeyDown(Input::Key::E)) {
		cameraMove.y -= deltaTime;
	}
	if (cameraMove != vec3(0)) {
		m_ViewChanged = true;
	}

	Position += Rotation * cameraMove * movespeed;

	if (!m_Input->IsMouseDown(Input::MouseButton::RIGHT))
	{
		m_Input->SetCursorMode(Input::CursorMode::NORMAL);
	}
	else
	{
		//printf("Last mouse: (%.0f,%.0f), curr mouse: (%.0f, %.0f)\n", m_LastMouse.x, m_LastMouse.y, mousePos.x, mousePos.y);
		m_Input->SetCursorMode(Input::CursorMode::LOCKED);
		float yaw = mouseMove.x;
		float pitch = mouseMove.y;
		if (yaw != 0.f || pitch != 0.f) {
			m_ViewChanged = true;
			mat3  deltaRot = eulerAngleXY(pitch, yaw);
			Rotation =  Rotation * deltaRot;
			ViewRotation = glm::inverse(Rotation);
		}
	}
	Dirty = true;
	m_LastMouse = mousePos;
}
