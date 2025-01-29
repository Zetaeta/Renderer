#pragma once

#include "core/Maths.h"

class Input
{
public:
	enum class Key : u32
	{
		W,
		A,
		S,
		D,
		Q,
		E
	};

	enum class MouseButton : u8
	{
		LEFT,
		MIDDLE,
		RIGHT
	};

	enum class CursorMode : u8
	{
		NORMAL = 0,
		HIDDEN = 1,
		LOCKED = 2
	};

	virtual bool IsKeyDown(Key key) = 0;
	virtual vec2 GetMousePosition() = 0;
	virtual vec2 GetWindowMousePos(){ return GetMousePosition(); };
	virtual ivec2 GetAbsoluteMousePos() = 0;
	virtual bool IsMouseDown(MouseButton mb) = 0;
	virtual void SetCursorMode(CursorMode mm) = 0;

	virtual bool IsAnyWindowFocused() const = 0;
};
