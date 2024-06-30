#pragma once
#include <memory>
#include <string>
#include "core/TypeInfoUtils.h"
#include "core/BaseObject.h"

struct Scene;
class SceneComponent;

class SceneObject : public BaseObject
{
	DECLARE_RTTI(SceneObject, BaseObject);
	SceneObject();
	SceneObject(Scene* scene, std::string n //, std::vector<SceneComponent> ms = {}
		);

	SceneObject(SceneObject&& other);
	SceneObject& operator=(SceneObject&& other);

	virtual ~SceneObject();

	void Initialize();

	virtual void OnInitialize() {}
	virtual void OnPreInitialize() {}

	virtual void Begin();

	bool ImGuiControls();

	SceneComponent* GetRoot()
	{
		return root.get();
	}

	Scene& GetScene()
	{
		return *m_Scene;
	}

	void SetScene(Scene* scene)
	{
		m_Scene = scene;
	}

	void Update();

	template <typename TComp, typename... TArgs>
	TComp& SetRoot(TArgs&&... args)
	{
		root = std::make_unique<TComp>(this, std::forward<TArgs>(args)...);
		OnSetRoot();
		return static_cast<TComp&>(*root);
	}

	Name const& GetName()
	{
		return m_Name;
	}
	void SetName(Name const& name)
	{
		m_Name = name;
	}

	Scene*		m_Scene = nullptr;
	std::string m_Name;
	// std::vector<SceneComponent> components;
	std::unique_ptr<SceneComponent> root;
	private:
	void OnSetRoot();
};

DECLARE_CLASS_TYPEINFO(SceneObject);
