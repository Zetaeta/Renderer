#pragma once
#include "Walnut/Application.h"
#include "Walnut/Image.h"
#include "RenderManager.h"
#include "Walnut/Timer.h"
#include "ImageRenderer.h"
#include "InputWalnut.h"

class RenderLayer : public Walnut::Layer, public ImageRenderMgr
{
public:
	RenderLayer()
		: ImageRenderMgr(new InputWalnut)
	{}

	virtual void OnUpdate(float deltaTime) {
		m_Camera.Tick(deltaTime);
	}


	virtual void OnUIRender() override
	{
		DrawUI();
	}

	void OnRenderStart() override
	{
		if ((m_Img == nullptr || m_Img->GetWidth() != m_ViewWidth || m_Img->GetHeight() != m_ViewHeight)) {
			m_Img = make_shared<Walnut::Image>(m_ViewWidth,m_ViewHeight, Walnut::ImageFormat::RGBA);
		}
	}

	void OnRenderFinish() override {
		if (m_ViewHeight * m_ViewHeight != 0) {
			m_Img->SetData(&m_ImgData[0]);
		}
		if (m_Img)
		{
			ImGui::Image(m_Img->GetDescriptorSet(), { static_cast<float>(m_ViewWidth), static_cast<float>(m_ViewHeight) }, { 0, 1 }, {1,0});
		}
	}

private:
	shared_ptr<Walnut::Image> m_Img;
};

