#pragma once
#include <core/Types.h>
#include "Viewport.h"
#include <core/Utils.h>
#include "ScreenObjManager.h"
#include <render/DeviceTexture.h>
#include <chrono>
#include <core/WeakPtr.h>

class Input;

class Editor
{
public:
	void OnClickPoint(ivec2 position);
	
	void OnLeftClick();
	void OnEndLeftClick();

	void OnRightClick();
	void OnEndRightClick() {}

	void OnWindowResize(u32 width, u32 height);

	void DrawControls();
	void DrawObjectsWindow();
	void DrawComponentsWindow();
	void DrawComponentsRecursive(SceneComponent* sceneComp);

	void Tick(float dt);

	Viewport* GetViewportAt(ivec2 position)
	{
		return mViewports.empty() ? nullptr : mViewports[0];
	}

	static Editor* Get()
	{
		return sSingleton;
	}

	static Editor* Create(Input* input, class RenderManager* rmg);

	static void Destroy();

	void SelectComponent(class SceneComponent* Component);

	void ClickComponent(class SceneComponent* Component);

	void CreateScreenIdTex(u32 width, u32 height);

	ScreenObjManager& GetScreenObjManager()
	{
		return ScreenObjMgr;
	}
	SceneComponent* GetSelectedComponent()
	{
		return ::Get(mSelectedComponent);
	}
	class SceneObject* GetSelectedObject();

	void SelectObject(const SceneObject* Obj);

private:
	bool mWasRightMouseDown = false;
	bool mWasMouseDown = false;
	Input* mInput;

	Vector<Viewport*> mViewports;
	Scene* mScene = nullptr;
	rnd::IDeviceTexture::Ref mScreenIdTex;

	RenderManager* mRmgr;
	Editor(Input* input, class RenderManager* rmg);
	static Editor* sSingleton;
	WeakPtr<class SceneComponent> mSelectedComponent = {};
	ScreenObjManager ScreenObjMgr;
	int mSelectedSceneObj = 0;

	std::chrono::time_point<std::chrono::system_clock> mLastSaved;
};
