#pragma once

#include "core/Maths.h"
#include "WinApi.h"

namespace wnd
{
class Window
{
public:
	Window(u32 width, u32 height, wchar_t const* name);
	virtual ~Window();

	virtual void Tick();

	virtual void Resize_WndProc(u32 resizeWidth, u32 resizeHeight);
	virtual void Move_WndProc(int posX, int posY) {}
	virtual void OnDestroy_WndProc();
protected:
	HWND mHwnd = (HWND) 0;
	u32 mWidth = 0;
	u32 mHeight = 0;

};
}
