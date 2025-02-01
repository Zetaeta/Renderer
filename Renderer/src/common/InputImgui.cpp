#include "InputImgui.h"
#include "WinUser.h"


static constexpr int sWinMouse[] =
{
	VK_LBUTTON,
	VK_MBUTTON,
	VK_RBUTTON
};

constexpr static int const sWinKeys[] = {
	0x57,
	0x41,
	0x53,
	0x44,
	0x51,
	0x45,
};

bool InputImgui::IsKeyDown(Key key)
{
	if (ImGui::IsAnyItemHovered())
	{
		return false;
	}
	return IsAnyWindowFocused() && (GetKeyState(sWinKeys[(int)key]) & 0x8000);
}

bool InputImgui::IsMouseDown(MouseButton mb)
{
	return IsAnyWindowFocused() && (GetKeyState(sWinMouse[(int)mb]) & 0x8000);
}

void InputImgui::GetCursorPosition(int* xpos, int* ypos)
{
	POINT pos;

	// ImGui::GetCursorPos()
	if (GetCursorPos(&pos))
	{
		ScreenToClient(m_Hwnd, &pos);

		if (xpos)
			*xpos = pos.x;
		if (ypos)
			*ypos = pos.y;
	}
}

glm::ivec2 InputImgui::GetAbsoluteMousePos()
{
	POINT pos;
	ivec2 result(0);
	if (GetCursorPos(&pos))
	{
		result.x = pos.x;
		result.y = pos.y;
	}
	return result;
}

bool InputImgui::IsAnyWindowFocused() const
{
	HWND focused = GetActiveWindow();
	HINSTANCE focusedProcess = (HINSTANCE) GetWindowLongPtr(focused, GWLP_HINSTANCE);
	return focusedProcess == GetModuleHandle(NULL);
}

glm::vec2 InputImgui::GetWindowMousePos()
{
	if (ImGui::GetWindowViewport() == ImGui::GetMainViewport())
	{
		ImVec2 viewportPos = ImGui::GetMainViewport()->Pos;
		return GetMousePosition() - vec2(viewportPos.x, viewportPos.y);
	}
	return {0,0};
}
