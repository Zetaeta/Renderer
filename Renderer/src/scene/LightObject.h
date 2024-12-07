#pragma once
#include "scene/SceneComponent.h"
#include "scene/Scene.h"

template<typename TLight>
class LightComponent : public SceneComponent
{
	DECLARE_RTTI(LightComponent, SceneComponent);

public:
	LightComponent() {
	
		if constexpr (TLight::HAS_DIRECTION)
		{
			SetTransform(TLight::GetDefaultTrans());
		}
	}
	
	template<typename TParent>
	LightComponent(TParent* parent)
		: SceneComponent(parent)
	{
		if constexpr (TLight::HAS_DIRECTION)
		{
			SetTransform(TLight::GetDefaultTrans());
		}
	}

	void OnInitialize()
	{
		ZE_ASSERT(!IsInitialized());
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
//template<typename TLight>
//DECLARE_CLASS_TYPEINFO_TEMPLATE(LightComponent<TLight>);


template <typename T>
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
				GetTypeName<T>());
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
		auto& ind = GetRoot()->AddChild<StaticMeshComponent>("Indicator");
		ind.SetType(StaticMeshComponent::EType::GADGET);
		ind.SetMesh(T::GADGET);
		ind.SetTransform(T::GADGET_TRANS);
		ind.SetScale(vec3(0.1f));
	}
};

//template<typename TLight>
//DECLARE_CLASS_TYPEINFO_TEMPLATE(LightObject<TLight>);
