#include "InputImgui.h"



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

glm::vec2 InputImgui::GetWindowMousePos()
{
	if (ImGui::GetWindowViewport() == ImGui::GetMainViewport())
	{
		ImVec2 viewportPos = ImGui::GetMainViewport()->Pos;
		return GetMousePosition() - vec2(viewportPos.x, viewportPos.y);
	}
	return {0,0};
}
