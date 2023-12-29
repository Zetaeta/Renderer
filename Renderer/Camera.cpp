#include "Camera.h"
#include "Walnut/Input/Input.h"
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

using Walnut::Input;

void Camera::Tick(float deltaTime)
{
	vec2 mousePos = Input::GetMousePosition();
	//std::cout << mousePos.x << ", " << mousePos.y << std::endl;
	vec2 mouseMove = (mousePos - m_LastMouse) * deltaTime * speed;
	m_LastMouse = mousePos;
	vec3 cameraMove{0};

	if (Walnut::Input::IsKeyDown(Walnut::KeyCode::W)) {
		cameraMove.z += deltaTime;
	}
	if (Walnut::Input::IsKeyDown(Walnut::KeyCode::S)) {
		cameraMove.z -= deltaTime;
	}
	if (Walnut::Input::IsKeyDown(Walnut::KeyCode::D)) {
		cameraMove.x += deltaTime;
	}
	if (Walnut::Input::IsKeyDown(Walnut::KeyCode::A)) {
		cameraMove.x -= deltaTime;
	}
	if (Walnut::Input::IsKeyDown(Walnut::KeyCode::Q)) {
		cameraMove.y += deltaTime;
	}
	if (Walnut::Input::IsKeyDown(Walnut::KeyCode::E)) {
		cameraMove.y -= deltaTime;
	}
	if (cameraMove != vec3(0)) {
		m_ViewChanged = true;
	}

	position += view * cameraMove;

	if (!Input::IsMouseButtonDown(Walnut::MouseButton::Right)) {
		Input::SetCursorMode(Walnut::CursorMode::Normal);
	}
	else
	{
		Input::SetCursorMode(Walnut::CursorMode::Locked);
		float yaw = mouseMove.x;
		float pitch = mouseMove.y;
		if (yaw != 0.f || pitch != 0.f) {
			m_ViewChanged = true;
			mat3  deltaRot = eulerAngleXY(-pitch, -yaw);
			rotation = deltaRot * rotation;
			view = glm::inverse(rotation);
		}
	}
}
