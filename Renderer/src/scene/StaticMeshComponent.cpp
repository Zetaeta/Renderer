#include "StaticMeshComponent.h"
#include "Scene.h"
#include "imgui.h"

StaticMeshComponent::StaticMeshComponent(AssetPath const& path)
	: m_Mesh(path)
{
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
		mPrimId = GetScene().DataInterface().AddPrimitive(this);
	}
	else
	{
		printf("Couldn't find mesh %s", m_Mesh.ToString().c_str());
	}
}

void StaticMeshComponent::OnDeinitialize()
{
	if (mPrimId != InvalidPrimId())
	{
		GetScene().DataInterface().RemovePrimitive(mPrimId);
		mPrimId = InvalidPrimId();
	}
}

void StaticMeshComponent::SetMesh(CompoundMesh::Ref mesh)
{
	m_MeshRef = mesh;
	if (IsValid(mesh))
	{
		m_Mesh = GetScene().GetAssetManager()->GetMesh(mesh).GetPath();
		printf("Set mesh %p with path %s\n", mesh.get(), m_Mesh.ToString().c_str());
		//m_MeshInst = GetScene().AddMesh(mesh);
	}

	MarkDirty();
}

void StaticMeshComponent::SetMesh(const AssetPath& path)
{
	m_Mesh = path;
	printf("Set mesh (%s)\n", m_Mesh.ToString().c_str());
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
	if (mPrimId != InvalidPrimId())
	{
		GetScene().DataInterface().UpdateTransform_MainThread(mPrimId, m_WorldTransform);
	}
	//MeshInstance* mesh = scene.GetMeshInstance(m_MeshInst);
	//if (mesh)
	//{
	//	mesh->trans = GetWorldTransform();
	//}
}

bool StaticMeshComponent::ImGuiControls()
{
	ImGui::Text(m_Mesh.ToString().c_str());
	return Super::ImGuiControls();
}

MaterialID StaticMeshComponent::GetMaterial(u32 meshPartIdx)
{
	if (mMaterialOverrides.size() > meshPartIdx)
	{
		return mMaterialOverrides[meshPartIdx];
	}
	if (IsValid(m_MeshRef) && m_MeshRef->components.size() > meshPartIdx)
	{
		return m_MeshRef->components[meshPartIdx].material;
	}
	return Material::DefaultMaterial;
}


void StaticMeshComponent::SetSelected(bool selected)
{
	Super::SetSelected(selected);
	if (mPrimId != InvalidPrimId())
	{
		GetScene().DataInterface().UpdateSelected_MainThread(mPrimId, selected);
	}
}

//DEFINE_CLASS_TYPEINFO(PrimitiveComponent)
//BEGIN_REFL_PROPS()
//END_REFL_PROPS()
//END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO(StaticMeshComponent)
BEGIN_REFL_PROPS()
REFL_PROP(m_Mesh)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

