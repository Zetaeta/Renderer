#pragma once

#include "SceneComponent.h"

namespace rnd
{
class IPrimitiveDrawer;
class IRenderDevice;

class IDrawable : public SelfDestructingRefCounted
{
public:
	virtual ~IDrawable() {}
	virtual void DrawDynamic(IPrimitiveDrawer& drawInterface) = 0;
	virtual void InitResources(IRenderDevice& device) {}
};
}


class DrawableComponent : public SceneComponent
{
	DECLARE_RTTI(DrawableComponent, SceneComponent);

	template<typename... Args>
	DrawableComponent(Args&&... args)
		: Super(std::forward<Args>(args)...)
	{
	}

	void OnInitialize() override;
	void OnDeinitialize() override;

	virtual bool ShouldAddToScene() { return false; }
	virtual RefPtr<rnd::IDrawable> CreateDrawable();

protected:
	PrimitiveId mPrimId = InvalidPrimId();
};
