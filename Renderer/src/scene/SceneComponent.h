#pragma once

#include "core/Types.h"
#include <memory>
#include "asset/Mesh.h"
#include "core/Transform.h"
#include "scene/SceneObject.h"
#include "Lights.h"
#include "core/TypeInfoUtils.h"
#include "AABB.h"
#include "scene/ScreenObject.h"

class Scene;
class SceneComponent;

//#define SC_CONSTRUCTORS()

class SceneComponent : public BaseSerialized, public std::enable_shared_from_this<SceneComponent>
{
	DECLARE_RTTI(SceneComponent, BaseSerialized);
public:
	using Transform = RotTransform;
	friend class SceneObject;

	RCOPY_PROTECT(SceneComponent);

	SceneComponent()
	{}
	virtual ~SceneComponent() {}

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

	using Owner = std::shared_ptr<SceneComponent>;
	using WeakRef = std::weak_ptr<SceneComponent>;
	Vector<Owner> const& GetChildren() const;

	void SetParent(SceneComponent* parent)
	{
		m_Parent = parent;
		mOuter = m_Object = parent->GetOwner();
	}


	SceneObject* GetOwner() const
	{
		return m_Object;
	}

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

	void Modify(bool allChildren);

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

	void Deinitialize()
	{
		OnDeinitialize();
		m_Initialized = false;
	}
	virtual void OnDeinitialize() {}

	ScreenObjectId GetScreenId() const { return mScreenId; }


 void ForAllChildren(std::function<void(BaseSerialized*)>, bool recursive = false) override;

protected:

	void SetOwner(SceneObject* owner)
	{
		m_Object = owner;
		mOuter = owner;
	}

	RefPtr<ScreenObject> mScreenObj;
	std::string			   m_Name;
	RotTransform			   m_Transform;
	std::vector<SceneComponent::Owner> m_Children;
	SceneComponent*		   m_Parent = nullptr;
	SceneObject*		   m_Object = nullptr;
	AABB aabb;
	ScreenObjectId			   mScreenId = {};

	Scene& GetScene()
	{
		return m_Object->GetScene();
	}

	WorldTransform m_WorldTransform;

	bool m_Dirty = true;
	bool m_AnyDirty = true;
	bool m_Initialized = false;

public:

#if ZE_BUILD_EDITOR
	bool Selected = false;
#endif


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

//class PrimitiveComponent : public SceneComponent
//{
//	DECLARE_RTTI(PrimitiveComponent, SceneComponent);
//};
//
//DECLARE_CLASS_TYPEINFO(PrimitiveComponent);

//template<typename T>
//struct TypeInfoAccessor
//{
//};



//using PointLightComponent = LightComponent<PointLight>;
//using SpotLightComponent = LightComponent<SpotLight>;

//using PointLightObject = LightObject<PointLight>;
//using DirLightObject = LightObject<DirLight>;
//using SpotLightObject = LightObject<SpotLight>;

