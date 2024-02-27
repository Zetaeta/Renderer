#pragma once

#include "Types.h"
#include <memory>
#include "Mesh.h"
#include "Transform.h"
#include "SceneObject.h"
#include "Lights.h"
#include "Scene.h"
#include "TypeInfo.h"

//struct Scene;
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
		SceneComponent::OwningPtr& child = m_Children.emplace_back(std::make_unique<TComp>(this, std::forward<TArgs>(args)...));
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
		SceneComponent::OwningPtr& child = m_Children.emplace_back(std::make_unique<TComp>(this));
		child->SetName(name);
		return static_cast<TComp&>(*child);
	}

	void SetParent(SceneComponent* parent)
	{
		m_Parent = parent;
		m_Object = parent->GetOwner();
	}

	using OwningPtr = std::unique_ptr<SceneComponent>;

	SceneObject* GetOwner()
	{
		return m_Object;
	}

	using WorldTransform = TTransform<quat>;

	WorldTransform GetWorldTransform()
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
	std::vector<OwningPtr> m_Children;
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

	virtual void OnUpdate(Scene& scene)
	{
		
		m_WorldTransform = static_cast<WorldTransform>(m_Transform);
		if (m_Parent != nullptr)
			m_WorldTransform =  m_Parent->GetWorldTransform() * m_WorldTransform;
	}
};

DECLARE_CLASS_TYPEINFO(SceneComponent)

inline bool IsValid(s32 ref)
{
	return ref >= 0;
}

class MeshComponent : public SceneComponent
{
	DECLARE_RTTI(MeshComponent, SceneComponent);
public:
	MeshComponent() {}

	MeshComponent(AssetPath const& path) :m_Mesh(path) {}
	
	template<typename TParent>
	MeshComponent(TParent* parent)
		:SceneComponent(parent), m_MeshInst(-1)
	{}
	MeshComponent(SceneObject* parent, MeshInstanceRef minst, String const& name = "", Transform const& trans = Transform{})
		: SceneComponent(parent, name, trans), m_MeshInst(minst)
	{}

	MeshComponent(SceneComponent* parent, MeshInstanceRef minst, String const& name = "", Transform const& trans = Transform{})
		: SceneComponent(parent, name, trans), m_MeshInst(minst)
	{}

	enum class EType : u8
	{
		VISIBLE = 0,
		GADGET = 1
	};

	void OnInitialize() override;

	void SetMesh(MeshRef mesh);
	void SetMesh(AssetPath path);

	virtual void	OnUpdate(Scene& scene) override;


	bool ImGuiControls() override;

	void SetType(EType type)
	{
		m_Type = type;
		MarkDirty();
	}

protected:
	EType m_Type = EType::VISIBLE;
	MeshInstanceRef m_MeshInst = -1;
	MeshRef m_MeshRef = -1;
	Name m_Mesh;
};
DECLARE_CLASS_TYPEINFO(MeshComponent)


template<typename TLight>
class LightComponent : public SceneComponent
{
	DECLARE_RTTI(LightComponent, SceneComponent);

public:
	LightComponent() {}
	
	template<typename TParent>
	LightComponent(TParent* parent)
		: SceneComponent(parent)
	{
	}

	void OnInitialize()
	{
		RASSERT(!IsInitialized());
		m_LightData.comp = this;
		m_Light = GetScene().AddLight<TLight>();
		OnUpdate(GetScene());
	}

	void Begin() override
	{
		Super::Begin();
		OnUpdate(GetScene());
	}

	void OnUpdate(Scene& scene) override
	{
		Super::OnUpdate(scene);
		if constexpr (TLight::HAS_POSITION)
		{
			m_LightData.SetPosition(GetWorldTransform().translation);
		}

		if constexpr (TLight::HAS_DIRECTION)
		{
			m_LightData.SetDirection(GetWorldTransform().GetRotation() * TLight::GetDefaultDir());
		}
		GetScene().GetLight<TLight>(m_Light) = m_LightData;
		
		if constexpr (std::is_same_v<TLight, SpotLight>)
		{
			auto transf = GetWorldTransform();
			transf.SetScale(vec3(1));
			//GetScene().GetLight<TLight>(m_Light).trans = transf;
		}
	}

	TLight::Ref m_Light;

	TLight m_LightData;
};
template<typename TLight>
DECLARE_CLASS_TYPEINFO_TEMPLATE(LightComponent<TLight>);


//template<typename T>
//struct TypeInfoAccessor
//{
//};


template<typename T>
class LightObject : public SceneObject
{
public:
	DECLARE_RTTI(LightObject, SceneObject);

	LightObject() {}

	LightObject(Scene* scene, RotTransform trans = RotTransform{}, String const& name = "")
		: SceneObject(scene, name)
	{
		if (m_Name.empty())
		{
			m_Name = scene->MakeName(
			GetTypeName<T>()
			);
		}
		SetupDefaults();
	}

	void OnPreInitialize()
	{
		if (root == nullptr)
		{
			SetupDefaults();
		}
	}

	void SetupDefaults()
	{
		SetRoot<LightComponent<T>>();
		auto& ind = GetRoot()->AddChild<MeshComponent>("Indicator");
		ind.SetType(MeshComponent::EType::GADGET);
		String mesh = T::GADGET;
		ind.SetMesh(mesh);
		ind.SetTransform(T::GADGET_TRANS);
		ind.SetScale(vec3(0.1f));
	}
};

template<typename TLight>
DECLARE_CLASS_TYPEINFO_TEMPLATE(LightObject<TLight>);

//using PointLightComponent = LightComponent<PointLight>;
//using SpotLightComponent = LightComponent<SpotLight>;

//using PointLightObject = LightObject<PointLight>;
//using DirLightObject = LightObject<DirLight>;
//using SpotLightObject = LightObject<SpotLight>;

