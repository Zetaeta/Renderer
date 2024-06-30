#include "scene/SceneComponent.h"
#include "scene/Scene.h"
#include "imgui.h"
#include "common/ImguiControls.h"


SceneComponent::SceneComponent(SceneComponent* parent, String const& name, Transform const& trans)
	: m_Parent(parent), m_Name(name), m_Transform(trans), m_Object(parent->GetOwner())
{
	m_WorldTransform = WorldTransform(m_Transform) * parent->GetWorldTransform();
}

Vector<SceneComponent::Owner> const& SceneComponent::GetChildren()
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

	bool dirty = rnd::ImGuiControls(ClassValuePtr::From(*this), false);

	if (dirty)
	{
		MarkDirty();
	}
	return dirty;
}

void SceneComponent::Initialize()
{
	for (auto const& child : m_Children)
	{
		child->SetParent(this);
		child->Initialize();
	}
	OnInitialize();
	m_Initialized = true;
}

void SceneComponent::OnUpdate(Scene& scene)
{

	m_WorldTransform = static_cast<WorldTransform>(m_Transform);
	if (m_Parent != nullptr)
		m_WorldTransform = m_Parent->GetWorldTransform() * m_WorldTransform;
}

void StaticMeshComponent::OnInitialize()
{
	if (!IsValid(m_MeshRef))
	{
		m_MeshRef = GetScene().GetAssetManager()->GetMesh(m_Mesh);
	}
	if (IsValid(m_MeshRef))
	{
		//if (!IsValid(m_MeshInst))
		//{
		//	m_MeshInst = GetScene().AddMesh(mesh);
		//}
	}
	else
	{
		printf("Couldn't find mesh %s", m_Mesh.c_str()); 
	}
}

void StaticMeshComponent::SetMesh(CompoundMesh::Ref mesh)
{
	m_MeshRef = mesh;
	if (IsValid(mesh))
	{
		m_Mesh = GetScene().GetAssetManager()->GetMesh(mesh).GetPath();
		printf("Set mesh %p with path %s\n", mesh.get(), m_Mesh.c_str());
		//m_MeshInst = GetScene().AddMesh(mesh);
	}

	MarkDirty();
}

void StaticMeshComponent::SetMesh(AssetPath path)
{
	m_Mesh = path;
	printf("Set mesh (%s)\n", m_Mesh.c_str());
	if (IsInitialized())
	{
		m_MeshRef = GetScene().GetAssetManager()->GetMesh(path);
		//if (IsValid(m_MeshRef) && !IsValid(m_MeshInst))
		//{
		//	m_MeshInst = GetScene().AddMesh(m_MeshRef);
		//}
	}
}

void StaticMeshComponent::OnUpdate(Scene& scene)
{
	SceneComponent::OnUpdate(scene);
	//MeshInstance* mesh = scene.GetMeshInstance(m_MeshInst);
	//if (mesh)
	//{
	//	mesh->trans = GetWorldTransform();
	//}
}

bool StaticMeshComponent::ImGuiControls()
{
	ImGui::Text(m_Mesh.c_str());
	return Super::ImGuiControls();
}


DEFINE_CLASS_TYPEINFO(SceneComponent)
BEGIN_REFL_PROPS()
REFL_PROP(m_Name)
REFL_PROP_SETTER(m_Transform, SetTransform, META(AutoExpand))
REFL_PROP(m_Children)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO(StaticMeshComponent)
BEGIN_REFL_PROPS()
REFL_PROP(m_Mesh)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

