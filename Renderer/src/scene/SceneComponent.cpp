#include "scene/SceneComponent.h"
#include "scene/Scene.h"
#include "imgui.h"
#include "common/ImguiControls.h"
#include <editor/Editor.h>


SceneComponent::SceneComponent(SceneComponent* parent, String const& name, Transform const& trans)
	: m_Parent(parent), m_Name(name), m_Transform(trans), m_Object(parent->GetOwner())
{
	m_WorldTransform = WorldTransform(m_Transform) * parent->GetWorldTransform();
}

Vector<SceneComponent::Owner> const& SceneComponent::GetChildren() const
{
	return m_Children;
}

bool SceneComponent::ImGuiControls()
{
	//ImGui::Text("Transform:");
	//bool dirty = rnd::ImGuiControls(m_Transform);
	//if (ImGui::TreeNode("Children"))
	//{
	//	for (auto& child : m_Children)
	//	{
	//		ImGui::PushID(&child);

	//		child->ImGuiControls();
	//		ImGui::PopID();
	//	}
	//	ImGui::TreePop();
	//}

	bool dirty = ::ImGuiControls(ClassValuePtr::From(*this), false);

	if (dirty)
	{
		MarkDirty();
	}
	return dirty;
}

void SceneComponent::Modify(bool allChildren)
{
	if (m_Parent)
	{
		m_Parent->Modify(false);
	}
	else
	{
		m_Object->Modify(false);
	}

	if (allChildren)
	{
		for (auto& child : m_Children)
		{
			child->Modify(true);
		}
	}
}

void SceneComponent::Initialize()
{
	//#if R_EDITOR
	if (Editor* editor = Editor::Get())
	{
		mScreenId = editor->GetScreenObjManager().Register<SOSceneComponent>(this);
	}
	//#endif
	for (auto const& child : m_Children)
	{
		child->SetParent(this);
		child->Initialize();
	}
	OnInitialize();
	m_Initialized = true;
}

void SceneComponent::ForAllChildren(std::function<void(BaseSerialized*)> callback, bool recursive /*= false*/)
{
	for (auto& child : m_Children)
	{
		callback(child.get());
		if (recursive)
		{
			child->ForAllChildren(callback, recursive);
		}
	}

}

void SceneComponent::SetSelected(bool selected)
{
	Selected = selected;
}

void SceneComponent::OnUpdate(Scene& scene)
{

	m_WorldTransform = static_cast<WorldTransform>(m_Transform);
	if (m_Parent != nullptr)
	{
		m_WorldTransform = m_Parent->GetWorldTransform() * m_WorldTransform;
	}
	


}

DEFINE_CLASS_TYPEINFO(SceneComponent)
BEGIN_REFL_PROPS()
REFL_PROP(m_Name)
REFL_PROP_SETTER(m_Transform, SetTransform, META(AutoExpand))
REFL_PROP(m_Children)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

