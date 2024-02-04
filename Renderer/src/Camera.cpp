#include "Camera.h"
#include "Walnut/Input/Input.h"
#include <glm/gtx/euler_angles.hpp>
#include <iostream>


void Camera::Tick(float deltaTime)
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

	position += view * cameraMove * movespeed;
	printf("det %f\n", determinant(view));

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
			mat3  deltaRot = eulerAngleXY(-pitch, -yaw);
			rotation = deltaRot * rotation;
			view = glm::inverse(rotation);
		}
	}
	m_LastMouse = mousePos;
}
