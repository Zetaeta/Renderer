#pragma once
#include "RenderManager.h"
#include "Walnut/Timer.h"

class ImageRenderMgr : public RenderManager
{
	
public:
	ImageRenderMgr(Input* input)
		: RenderManager(input) {}

	void DrawUI() {
		ImGui::Begin("Controls");
		DrawFrameData();
		ImGui::Checkbox("paused", &m_Paused);
		SceneControls();
		ImGui::End();

		ImGui::Begin("Assets");
		m_AssMan.DrawControls();
		ImGui::End();


		ImGui::Begin("Viewport");
		u32 width = static_cast<int>(ImGui::GetContentRegionAvail().x);
		u32 height = static_cast<int>(ImGui::GetContentRegionAvail().y);
		if (width != m_ViewWidth || m_ViewHeight != height)
		{
			m_SizeChanged = true;
		}
		m_ViewWidth = width;
		m_ViewHeight = height;
		Render();
		ImGui::End();
	}

	virtual void DrawFrameData()
	{
		ImGui::Text("Frame time: %.3fms", m_FrameTime);
	}

	void Render() {

		OnRenderStart();

		if (m_ViewHeight * m_ViewHeight == 0) {
			return;
		}

		m_ImgData.resize(m_ViewWidth * m_ViewHeight);
		m_Renderer->Resize(m_ViewWidth, m_ViewHeight, &m_ImgData[0]);
		if (!m_Paused)
		{
			m_Renderer->Render(mScene);
		}
		OnRenderFinish();
		m_FrameTime = m_Timer.ElapsedMillis();
		m_Timer.Reset();
	}

	virtual void OnRenderStart() {}
	virtual void OnRenderFinish() {}

	float m_FrameTime = 0;
	vector<u32> m_ImgData;
	u32 m_ViewWidth;
	u32 m_ViewHeight;
	Walnut::Timer m_Timer;

	bool m_SizeChanged = true;
	bool m_Paused = true;
};
