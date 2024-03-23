#pragma once
#include "SceneComponent.h"
#include "imgui.h"

namespace rnd
{
inline bool ImGuiControls(SceneObject& so)
{
	return so.ImGuiControls();
}

bool ImGuiControls(TTransform<quat>& trans);

inline bool ImGuiControls(TTransform<Rotator>& trans)
{
	bool edited = false;
	edited |= ImGui::DragFloat3("translation", &trans.translation[0], 0.1f);
	edited |= ImGui::DragFloat3("scale", &trans.scale[0], 0.1f);
	edited |= ImGui::DragFloat3("rotation", &trans.rotation[0], 1.f);
	return edited;

}

bool ImGuiControls(ClassValuePtr cls, bool isConst);

}
