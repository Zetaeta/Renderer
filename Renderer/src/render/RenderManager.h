#pragma once
#include "stb_image.h"
#include "asset/Texture.h"
#include <memory>
#include "Renderer.h"
#include "scene/Scene.h"
#include <filesystem>
#include "scene/UserCamera.h"
#include "RastRenderer.h"
#include "asset/ObjReader.h"
#include <iostream>
#include "imgui.h"
#include "render/dx11/DX11Renderer.h"
#include <ranges>
#include <numeric>
#include "assimp/Importer.hpp"
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h> // Post processing flags

using namespace glm;
namespace fs = std::filesystem;

namespace rnd
{
	class IRenderDeviceCtx;
}

namespace ch = std::chrono;
class Timer
{
public:
	void Reset()
	{
		m_StartTime = std::chrono::system_clock::now();
	}

	double ElapsedMillis()
	{
		std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
		return double(std::chrono::duration_cast<ch::milliseconds>(endTime - m_StartTime).count());
	}

	std::chrono::time_point<std::chrono::system_clock> m_StartTime;
};

class RenderManager
{
public:
	RenderManager(Input* input);

	void CreateStarterScene();
	void CreateInitialScene();
	void CreateScene();

	bool LoadScene(String const& path);
	void SaveScene(String const& path);

	void SceneControls();

	void DrawUI();

	virtual void DrawFrameData();

	void Render();

	virtual void OnRenderStart() {}
	virtual void OnRenderFinish() {}

	virtual IRenderer* Renderer() { return m_Renderer.get(); }
	virtual rnd::IRenderDeviceCtx* DeviceCtx() { return nullptr; }
	virtual rnd::IRenderDevice* Device() { return nullptr; }

	Scene mScene;
protected:
	void TypeSelector(ClassTypeInfo const& cls);

	ClassTypeInfo const* m_SelectedSO = nullptr;

	UserCamera m_Camera;
	std::unique_ptr<IRenderer> m_Renderer;
	AssetManager m_AssMan;
	float m_FrameTime = 0;
	Timer m_Timer;
};
