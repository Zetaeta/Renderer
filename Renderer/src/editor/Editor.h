#pragma once
#include <core/Types.h>
#include "Viewport.h"
#include <core/Utils.h>
#include "ScreenObjManager.h"

class Input;

class Editor
{
public:
	void OnClickPoint(ivec2 position);
	
	void OnLeftClick();
	void OnEndLeftClick();

	void OnWindowResize(u32 width, u32 height);

	void Tick(float dt);

	Viewport* GetViewportAt(ivec2 position)
	{
		return mViewports.empty() ? nullptr : &mViewports[0];
	}

	static Editor* Get()
	{
		return sSingleton;
	}

	static Editor* Create(Input* input, class RenderManager* rmg)
	{
		RASSERT(sSingleton == nullptr);
		return (sSingleton = new Editor(input, rmg));
	}

	void SelectComponent(class SceneComponent* Component);

	ScreenObjManager& GetScreenObjManager()
	{
		return ScreenObjMgr;
	}

private:
	bool mWasMouseDown = false;
	Input* mInput;

	Vector<Viewport> mViewports;

	Editor(Input* input, class RenderManager* rmg);
	static Editor* sSingleton;
	class SceneComponent* SelectedComponent;
	ScreenObjManager ScreenObjMgr;
};
