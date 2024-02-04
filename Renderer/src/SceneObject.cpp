#include "SceneObject.h"
#include "imgui.h"
#include "Scene.h"
#include "SceneComponent.h"

 SceneObject::SceneObject(Scene* scene, std::string n /*, std::vector<SceneComponent> ms = {} */)
	: m_Scene(scene), m_Name(n), root(make_unique<SceneComponent>(this, n + "Root")) //, components(ms)
{
}

 SceneObject::SceneObject(SceneObject&& other)
{
	*this = std::move(other);
}

SceneObject& SceneObject::operator=(SceneObject&& other)
{
	m_Scene = (other.m_Scene);
	m_Name = (std::move(other.m_Name));
	root = std::move(other.root);
	return *this;
}

SceneObject::~SceneObject()
{
}

void SceneObject::Begin()
{
	root->Begin();
}

bool SceneObject::ImGuiControls()
{
	ImGui::Text(m_Name.c_str());
	return root->ImGuiControls();
}
