#pragma once
#include "core/Maths.h"
#include "common/Input.h"
#include "core/Utils.h"
#include "Camera.h"

namespace wnd { class Window; }

class Tickable
{
public:
	Tickable();
	~Tickable();
	static void TickAll(float deltaTime);
	virtual void Tick(float deltaTime) = 0;
private:
	static Vector<Tickable*> sTickables;
};

class UserCamera : public Camera, Tickable
{
public:
	UserCamera(Input* input, wnd::Window* owningWindow = nullptr)
	:Camera(ECameraType::PERSPECTIVE), m_Input(input), mWindow(owningWindow){}

	void Tick(float deltaTime) override;

	bool ViewChanged() { return m_ViewChanged; }
	void CleanView() {m_ViewChanged = false;}


	float movespeed = .005f;
	float turnspeed = .001f;
private:
	ivec2 m_LastMouse;
	bool m_ViewChanged;
	Input* m_Input;
	wnd::Window* mWindow = nullptr;
};
