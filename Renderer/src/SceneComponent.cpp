#include "SceneComponent.h"
#include "Scene.h"
#include "imgui.h"
#include "ImguiControls.h"


SceneComponent::SceneComponent(SceneComponent* parent, String const& name, Transform const& trans)
	: m_Parent(parent), m_Name(name), m_Transform(trans), m_Object(parent->GetOwner())
{
	m_WorldTransform = WorldTransform(m_Transform) * parent->GetWorldTransform();
}

bool SceneComponent::ImGuiControls()
{
	ImGui::Text("Transform:");
	bool dirty = rnd::ImGuiControls(m_Transform);
	if (ImGui::TreeNode("Children"))
	{
		for (auto& child : m_Children)
		{
			ImGui::PushID(&child);

			child->ImGuiControls();
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	if (dirty)
	{
		MarkDirty();
	}
	return dirty;
}

void MeshComponent::SetMesh(MeshRef mesh)
{
	m_Mesh = mesh;
	if (IsValid(mesh) && !IsValid(m_MeshInst))
	{
		m_MeshInst = GetScene().AddMesh(mesh);
	}

	MarkDirty();
}

void MeshComponent::OnUpdate(Scene& scene)
{
	SceneComponent::OnUpdate(scene);
	MeshInstance* mesh = scene.GetMeshInstance(m_MeshInst);
	if (mesh)
	{
		mesh->trans = GetWorldTransform();
		mesh->mesh = m_Mesh;
	}
}
