#pragma once
#include <memory>
#include <string>
#include "core/TypeInfoUtils.h"
#include <core/BaseSerialized.h>

class Scene;
class SceneComponent;

class SceneObject : public BaseSerialized, public std::enable_shared_from_this<SceneObject>
{
	DECLARE_RTTI(SceneObject, BaseSerialized);
	SceneObject();
	SceneObject(Scene* scene, Name n //, std::vector<SceneComponent> ms = {}
		);

	SceneObject(SceneObject&& other);
	SceneObject& operator=(SceneObject&& other);

	virtual ~SceneObject();

	void Initialize();

	virtual void OnInitialize() {}
	virtual void OnPreInitialize() {}

	virtual void Begin();

	bool ImGuiControls();

	SceneComponent* GetRoot() const
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

	void Modify(bool bAllChildren = false, bool modified = true) override;

	void Update();

	template <typename TComp, typename... TArgs>
	TComp& SetRoot(TArgs&&... args)
	{
		root = std::make_shared<TComp>(this, std::forward<TArgs>(args)...);
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
	Name m_Name;
	// std::vector<SceneComponent> components;
	std::shared_ptr<SceneComponent> root;
	private:
	void OnSetRoot();

public:
	void ForAllChildren(std::function<void(BaseSerialized*)> callback, bool recursive = false) override;
};

DECLARE_CLASS_TYPEINFO(SceneObject);
