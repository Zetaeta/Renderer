#pragma once
#include <core/RefCounted.h>

using ScreenObjectId = u32;

constexpr ScreenObjectId SO_NONE = 0;

class SceneComponent;

class ScreenObject : public RefCountedObject
{
public:
	virtual void OnClicked() {};
	ScreenObjectId Id;
};

class SOSceneComponent : public ScreenObject
{
public:
	SOSceneComponent(SceneComponent* sc)
		: mSceneComp(sc) {}
	virtual void OnClicked() override;

protected:
	SceneComponent* mSceneComp;
};

