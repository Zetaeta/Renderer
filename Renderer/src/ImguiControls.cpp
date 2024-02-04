#include "ImguiControls.h"



bool rnd::ImGuiControls(TTransform<quat>& trans)
{
	bool edited = false;
	edited |= ImGui::DragFloat3("translation", &trans.translation[0], 0.1f);
	edited |= ImGui::DragFloat3("scale", &trans.scale[0], 0.1f);
	if (ImGui::DragFloat4("rotation", &trans.rotation[0], 0.05f))
	{
		edited = true;
		trans.rotation = normalize(trans.rotation);
	}
	return edited;
}
