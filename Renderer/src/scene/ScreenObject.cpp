#include "ScreenObject.h"
#include "editor/Editor.h"


void SOSceneComponent::OnClicked()
{
	Editor::Get()->SelectComponent(mSceneComp);
}
