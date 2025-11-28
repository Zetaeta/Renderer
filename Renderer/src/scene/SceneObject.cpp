#include "scene/SceneObject.h"
#include "imgui.h"
#include "scene/Scene.h"
#include "scene/SceneComponent.h"

 SceneObject::SceneObject(Scene* scene, HashString n /*, std::vector<SceneComponent> ms = {} */)
	: m_Scene(scene), m_Name(n), root(make_unique<SceneComponent>(this, "DefaultRoot")) //, components(ms)
{
}

 SceneObject::SceneObject(SceneObject&& other)
{
	*this = std::move(other);
}

 SceneObject::SceneObject()
{
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

void SceneObject::Initialize()
{
	OnPreInitialize();
	if (root)
	{
		root->SetOwner(this);
		root->Initialize();
	}
	OnInitialize();
}

void SceneObject::Begin()
{
	root->Begin();
}

bool SceneObject::ImGuiControls()
{
	ImGui::Text(m_Name.c_str());
	if (root)
	{
		return root->ImGuiControls();
	}
	else
	{
		return false;
	}
}

void SceneObject::Modify(bool bAllChildren, bool modified)
{
	if (modified)
	{
		m_Scene->Modify(false);
		if (bAllChildren)
		{
			root->Modify(true);
		}
	}

	Super::Modify(bAllChildren, modified);
}

void SceneObject::Update()
{
	if (root != nullptr)
	{
		root->Update(*m_Scene);
	}
}

void SceneObject::OnSetRoot()
{
	root->SetOwner(this);
}

void SceneObject::ForAllChildren(std::function<void(BaseSerialized*)> callback, bool recursive /*= false*/)
{
	callback(root.get());
	if (recursive)
	{
		root->ForAllChildren(callback, true);
	}
}

DEFINE_CLASS_TYPEINFO(SceneObject)
BEGIN_REFL_PROPS()
REFL_PROP(m_Name)
REFL_PROP(root, META(AutoExpand))
END_REFL_PROPS()
END_CLASS_TYPEINFO()

