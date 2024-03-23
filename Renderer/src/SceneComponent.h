#pragma once

#include "Types.h"
#include <memory>
#include "Mesh.h"
#include "Transform.h"
#include "SceneObject.h"
#include "Lights.h"
//#include "Scene.h"
#include "TypeInfo.h"

struct Scene;
class SceneComponent;

//#define SC_CONSTRUCTORS()

class SceneComponent : public BaseObject
{
	DECLARE_RTTI(SceneComponent, BaseObject);
public:
	using Transform = RotTransform;
	friend class SceneObject;

	RCOPY_PROTECT(SceneComponent);

	SceneComponent()
	{}

	SceneComponent(SceneObject* parent, String const& name = "", RotTransform const& trans = RotTransform{})
		: m_Object(parent), m_Name(name), m_Transform(trans)
	{
		m_WorldTransform = WorldTransform(m_Transform);
	}

	SceneComponent(SceneComponent* parent, String const& name = "", RotTransform const& trans = RotTransform{});

	template <typename TComp, typename... TArgs>
	TComp& AddChild(String name, TArgs&&... args)
	{
		auto& child = m_Children.emplace_back(std::make_unique<TComp>(this, std::forward<TArgs>(args)...));
		child->SetName(name);
		if (IsInitialized())
		{
			child->Initialize();
		}
		return static_cast<TComp&>(*child);
	}

	template<typename TComp>
	TComp& AddChild(Name name)
	{
		auto& child = m_Children.emplace_back(std::make_unique<TComp>(this));
		child->SetName(name);
		return static_cast<TComp&>(*child);
	}

	using Owner = std::unique_ptr<SceneComponent>;
	Vector<Owner> const& GetChildren();

	void SetParent(SceneComponent* parent)
	{
		m_Parent = parent;
		m_Object = parent->GetOwner();
	}


	SceneObject* GetOwner()
	{
		return m_Object;
	}

	using WorldTransform = TTransform<quat>;

	WorldTransform const& GetWorldTransform() const
	{
		return m_WorldTransform;
	}

	virtual bool ImGuiControls();

	void Update(Scene& scene)
	{
		if (m_Dirty)
		{
			OnUpdate(scene);
			//m_Dirty = false;
		}
		if (m_AnyDirty || m_Dirty)
		{
			for (auto& child : m_Children)
			{
				child->Update(scene);
			}
		}
	}

	virtual void Begin()
	{
		for (auto& child : m_Children)
		{
			child->Begin();
		}
	}

	void SetName(Name name)
	{
		m_Name = name;
	}

	void SetScale(vec3 scale)
	{
		m_Transform.SetScale(scale);
		MarkDirty();
	}

	void SetTransform(RotTransform const& trans)
	{
		m_Transform = trans;
		MarkDirty();
	}

	bool IsInitialized() { return m_Initialized; }

	void Initialize();
	virtual void OnInitialize() {}

protected:

	void SetOwner(SceneObject* owner)
	{
		m_Object = owner;
	}

	std::string			   m_Name;
	RotTransform			   m_Transform;
	std::vector<Owner> m_Children;
	SceneComponent*		   m_Parent = nullptr;
	SceneObject*		   m_Object = nullptr;

	Scene& GetScene()
	{
		return m_Object->GetScene();
	}

	WorldTransform m_WorldTransform;

	bool m_Dirty = true;
	bool m_AnyDirty = true;
	bool m_Initialized = false;

	void MarkDirty()
	{
		return;
		m_Dirty = true;
		m_Parent->MarkDirty();
		for (auto& child : m_Children)
		{
			child->MarkDirty();
		}
	}

	void MarkAnyDirty()
	{
		m_AnyDirty = true;
		m_Parent->MarkAnyDirty();
	}

	virtual void OnUpdate(Scene& scene);
};

DECLARE_CLASS_TYPEINFO(SceneComponent)

class StaticMeshComponent : public SceneComponent
{
	DECLARE_RTTI(StaticMeshComponent, SceneComponent);
public:
	StaticMeshComponent() {}

	StaticMeshComponent(AssetPath const& path) :m_Mesh(path) {}
	
	template<typename TParent>
	StaticMeshComponent(TParent* parent)
		:SceneComponent(parent)//, m_MeshInst(-1)
	{}
	//StaticMeshComponent(SceneObject* parent, MeshInstanceRef minst, String const& name = "", Transform const& trans = Transform{})
	//	: SceneComponent(parent, name, trans), m_MeshInst(minst)
	//{}

	//StaticMeshComponent(SceneComponent* parent, MeshInstanceRef minst, String const& name = "", Transform const& trans = Transform{})
	//	: SceneComponent(parent, name, trans), m_MeshInst(minst)
	//{}

	enum class EType : u8
	{
		VISIBLE = 0,
		GADGET = 1
	};

	void OnInitialize() override;

	void SetMesh(CompoundMesh::Ref mesh);
	void SetMesh(AssetPath path);

	virtual void	OnUpdate(Scene& scene) override;

	CompoundMesh* GetMesh() const { return m_MeshRef.get(); }

	bool ImGuiControls() override;

	void SetType(EType type)
	{
		m_Type = type;
		MarkDirty();
	}

protected:
	EType m_Type = EType::VISIBLE;
//	MeshInstanceRef m_MeshInst = -1;
	CompoundMesh::Ref m_MeshRef = CompoundMesh::INVALID_REF;
	Name m_Mesh;
};
DECLARE_CLASS_TYPEINFO(StaticMeshComponent)



//template<typename T>
//struct TypeInfoAccessor
//{
//};



//using PointLightComponent = LightComponent<PointLight>;
//using SpotLightComponent = LightComponent<SpotLight>;

//using PointLightObject = LightObject<PointLight>;
//using DirLightObject = LightObject<DirLight>;
//using SpotLightObject = LightObject<SpotLight>;

