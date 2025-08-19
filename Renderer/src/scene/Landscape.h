#pragma once
#include "SceneObject.h"
#include "render/RenderMaterial.h"
#include "DrawableComponent.h"

class Texture;

class Landscape : public SceneObject
{
	DECLARE_RTTI(Landscape, SceneObject);
public:
	Landscape() = default;
	Landscape(Scene* scene, uvec2 extents = uvec2{1, 1});
private:
	std::shared_ptr<Texture> mHeightmap;
	std::shared_ptr<rnd::RenderMaterial> mMaterial;
	friend class LandscapeComponent;
};

class LandscapeComponent : public DrawableComponent
{
	DECLARE_RTTI(LandscapeComponent, DrawableComponent);

public:
	template<typename... Args>
	LandscapeComponent(Args&&... args)
		: Super(std::forward<Args>(args)...)
	{
	}
	bool ShouldAddToScene() override;

	RefPtr<rnd::IDrawable> CreateDrawable() override;
};
