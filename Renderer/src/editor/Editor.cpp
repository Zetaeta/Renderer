#include "Editor.h"
#include <common/Input.h>
#include <render/RenderManager.h>


Editor::Editor(Input* input, RenderManager* rmg)
{
	
}

Editor* Editor::sSingleton;

void Editor::OnClickPoint(ivec2 position)
{
	Viewport* viewport = GetViewportAt(position);
	if (!viewport)
	{
		return;
	}


}

void Editor::OnLeftClick()
{
	vec2 mousePos = mInput->GetMousePosition();
	OnClickPoint({NumCast<int>(mousePos.x), NumCast<int>(mousePos.y)});
}

void Editor::OnEndLeftClick()
{
}

void Editor::OnWindowResize(u32 width, u32 height)
{
}

void Editor::Tick(float dt)
{
	bool isMouseDown = mInput->IsMouseDown(Input::MouseButton::LEFT);
	if (isMouseDown != mWasMouseDown)
	{
		if (isMouseDown)
		{
			OnLeftClick();
		}
		else
		{
			OnEndLeftClick();
		}
	}
}

void Editor::SelectComponent(SceneComponent* Component)
{
	SelectedComponent = Component;
}
