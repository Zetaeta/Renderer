#include "ScreenObject.h"
#include "editor/Editor.h"


void SOSceneComponent::OnClicked()
{
	Editor::Get()->ClickComponent(mSceneComp);
}
