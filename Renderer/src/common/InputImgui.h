#pragma once
#include "../../Walnut/vendor/imgui/imgui.h"
#include "Input.h"
#include <windows.h>

class InputImgui : public Input
{
public:
	constexpr static ImGuiKey const keys[] = {
		ImGuiKey_W,
		ImGuiKey_A,
		ImGuiKey_S,
		ImGuiKey_D,
		ImGuiKey_Q,
		ImGuiKey_E,
	};

	constexpr static ImGuiMouseButton const mbs[] = {
		ImGuiMouseButton_Left,
		ImGuiMouseButton_Middle,
		ImGuiMouseButton_Right,
	};

	InputImgui(//ImageRenderMgr* renderMgr
	)
		//: m_RenderMgr(renderMgr)
		{}

	bool IsKeyDown(Key key) override
	{
		if (ImGui::IsAnyItemHovered())
		{
			return false;
		}
		return ImGui::IsKeyDown(keys[(int)key]);
	}

	vec2 GetMousePosition() override
	{
		auto iv2 = ImGui::GetMousePos();
		return vec2(iv2.x, iv2.y);
	}

	bool IsMouseDown(MouseButton mb) override
	{
		return ImGui::IsMouseDown(mbs[(int)mb]);
	}

	void GetCursorPosition(int* xpos, int* ypos);

	void SetCursorMode(CursorMode mm)
	{
		if (m_CursorMode == mm)
		{
			return;
		}
		m_CursorMode = mm;
		if (mm == Input::CursorMode::LOCKED)
		{
			GetCursorPosition(&m_CursorX, &m_CursorY);
			//printf("Saved cursor pos at %i, %i\n", m_CursorX, m_CursorY);
			// ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			SetCursor(NULL);
			// SetCursorPos(m_RenderMgr->m_ViewWidth/2, m_RenderMgr->m_ViewHeight/2);
			UpdateClipRect(true);
		}
		else
		{
			//printf("Resetting cursor pos to %i, %i\n", m_CursorX, m_CursorY);
			// ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
			SetCursor(LoadCursorW(NULL, IDC_ARROW));
			SetCursorPos(m_CursorX, m_CursorY);
			UpdateClipRect(false);
		}
		return;
	}

	void UpdateClipRect(bool restrictToWindow)
	{
		RECT clipRect;
		GetClientRect(m_Hwnd, &clipRect);
		ClientToScreen(m_Hwnd, (POINT*)&clipRect.left);
		ClientToScreen(m_Hwnd, (POINT*)&clipRect.right);
		ClipCursor(&clipRect);
	}


 vec2 GetWindowMousePos() override;

private:
	CursorMode		m_CursorMode;
	HWND			m_Hwnd;
	int				m_CursorX;
	int				m_CursorY;
	//ImageRenderMgr* m_RenderMgr;
};
