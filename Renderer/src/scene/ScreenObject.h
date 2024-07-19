#pragma once
#include <core/RefCounted.h>

using ScreenObjectId = u32;

class ScreenObject : public RefCountedObject
{
	virtual void OnClicked() {};
	ScreenObjectId Id;
};

class SOSceneComponent : public ScreenObject
{
	virtual void OnClicked() override;
	class SceneComponent* mSceneComp;
};

