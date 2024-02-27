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

void MeshComponent::OnInitialize()
{
	MeshRef mesh = GetScene().GetAssetManager()->GetMesh(m_Mesh);
	m_MeshRef = mesh;
	if (IsValid(mesh))
	{
		if (!IsValid(m_MeshInst))
		{
			m_MeshInst = GetScene().AddMesh(mesh);
		}
	}
	else
	{
		printf("Couldn't find mesh %s", m_Mesh.c_str()); 
	}
}

void MeshComponent::SetMesh(MeshRef mesh)
{
	m_MeshRef = mesh;
	if (IsValid(mesh) && !IsValid(m_MeshInst))
	{
		m_Mesh = GetScene().GetAssetManager()->GetMesh(mesh).GetPath();
		printf("Set mesh with path %s\n", m_Mesh.c_str());
		m_MeshInst = GetScene().AddMesh(mesh);
	}

	MarkDirty();
}

void MeshComponent::SetMesh(AssetPath path)
{
	m_Mesh = path;
	printf("Set mesh (%s)\n", m_Mesh.c_str());
	if (IsInitialized())
	{
		m_MeshRef = GetScene().GetAssetManager()->GetMesh(path);
		if (IsValid(m_MeshRef) && !IsValid(m_MeshInst))
		{
			m_MeshInst = GetScene().AddMesh(m_MeshRef);
		}
	}
}

void MeshComponent::OnUpdate(Scene& scene)
{
	SceneComponent::OnUpdate(scene);
	MeshInstance* mesh = scene.GetMeshInstance(m_MeshInst);
	if (mesh)
	{
		mesh->trans = GetWorldTransform();
		mesh->mesh = m_MeshRef;
	}
}

bool MeshComponent::ImGuiControls()
{
	ImGui::Text(m_Mesh.c_str());
	return Super::ImGuiControls();
}


DEFINE_CLASS_TYPEINFO(SceneComponent)
BEGIN_REFL_PROPS()
REFL_PROP(m_Name)
REFL_PROP(m_Transform)
REFL_PROP(m_Children)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO(MeshComponent)
BEGIN_REFL_PROPS()
REFL_PROP(m_Mesh)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

#define LIGHTS \
	X(DirLight)\
	X(PointLight)\
	X(SpotLight)



#define X(l)\
	DEFINE_CLASS_TYPEINFO_TEMPLATE(template<>, LightComponent<l>)\
	BEGIN_REFL_PROPS()\
	REFL_PROP(m_LightData)\
	END_REFL_PROPS()\
	END_CLASS_TYPEINFO()

LIGHTS

#undef X

#define X(l)\
	DEFINE_CLASS_TYPEINFO_TEMPLATE(template<>, LightObject<l>)\
	BEGIN_REFL_PROPS()\
	END_REFL_PROPS()\
	END_CLASS_TYPEINFO()

LIGHTS

#undef X
