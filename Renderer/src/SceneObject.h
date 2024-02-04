#pragma once
#include <memory>
#include <string>

struct Scene;
class SceneComponent;

struct SceneObject
{
	SceneObject(Scene* scene, std::string n //, std::vector<SceneComponent> ms = {}
		);

	SceneObject(SceneObject&& other);
	SceneObject& operator=(SceneObject&& other);

	virtual ~SceneObject();

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

	template <typename TComp, typename... TArgs>
	TComp& SetRoot(TArgs&&... args)
	{
		root = std::make_unique<TComp>(this, std::forward<TArgs>(args)...);
		return static_cast<TComp&>(*root);
	}

	Scene*		m_Scene;
	std::string m_Name;
	// std::vector<SceneComponent> components;
	std::unique_ptr<SceneComponent> root;
};
