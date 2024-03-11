#pragma once
#include "stb_image.h"
#include "Texture.h"
#include <memory>
#include "Renderer.h"
#include "Scene.h"
#include <filesystem>
#include "Camera.h"
#include "RastRenderer.h"
#include "ObjReader.h"
#include <iostream>
#include "imgui.h"
#include "DX11Renderer.h"
#include <ranges>
#include <numeric>
#include "assimp/Importer.hpp"
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h> // Post processing flags

using namespace std;
using namespace glm;
namespace fs = std::filesystem;

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

	void SceneControls();

	void DrawUI() {
		ImGui::Begin("Controls");
		DrawFrameData();
		SceneControls();
		ImGui::End();

		if (Renderer())
		{
			Renderer()->DrawControls();
		}

		ImGui::Begin("Assets");
		m_AssMan.DrawControls();
		ImGui::End();


		Render();
	}

	virtual void DrawFrameData()
	{
		ImGui::Text("Frame time: %.3fms", m_FrameTime);
	}

	void Render() {

		OnRenderStart();

		//m_Renderer->Render(scene);

		OnRenderFinish();
		m_FrameTime = m_Timer.ElapsedMillis();
		m_Timer.Reset();
	}

	virtual void OnRenderStart() {}
	virtual void OnRenderFinish() {}

	virtual IRenderer* Renderer() { return m_Renderer.get(); }

protected:
	void TypeSelector(ClassTypeInfo const& cls);

	ClassTypeInfo const* m_SelectedSO = nullptr;

	Camera m_Camera;
	unique_ptr<IRenderer> m_Renderer;
	Scene scene;
	AssetManager m_AssMan;
	float m_FrameTime = 0;
	Timer m_Timer;
};
