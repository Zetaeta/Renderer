#pragma once
#include "Input.h"
#include "Walnut/Input/KeyCodes.h"
#include "Walnut/Input/Input.h"

class InputWalnut : public Input
{
	constexpr static const Walnut::KeyCode keys[] =
	{
		Walnut::KeyCode::W,
		Walnut::KeyCode::A,
		Walnut::KeyCode::S,
		Walnut::KeyCode::D,
		Walnut::KeyCode::Q,
		Walnut::KeyCode::E
	};
	constexpr static const Walnut::MouseButton mbs[] =
	{
		Walnut::MouseButton::Left,
		Walnut::MouseButton::Middle,
		Walnut::MouseButton::Right,
	};
	bool IsKeyDown(Key key) override
	{
		return Walnut::Input::IsKeyDown(keys[(int) key]);
	}

	vec2 GetMousePosition() override
	{
		return Walnut::Input::GetMousePosition();
	}

	bool IsMouseDown(MouseButton mb)
	{
		return Walnut::Input::IsMouseButtonDown(mbs[(int) mb]);
	}

	void SetCursorMode(CursorMode mm)
	{
		Walnut::Input::SetCursorMode(static_cast<Walnut::CursorMode>(mm));
	}
};
