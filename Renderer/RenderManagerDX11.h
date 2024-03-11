#pragma once
#include "src/ImageRenderMgrDX11.h"
class RenderManagerDX11 : public RenderManager
{
public:
	using Super = RenderManager;


	RenderManagerDX11(ID3D11Device* device, ID3D11DeviceContext* context)
		: Super(new InputImgui())
	{
		m_hardwareRenderer = std::make_unique<DX11Renderer>(&m_Camera, 0, 0, device, context);
	}
	
	virtual void DrawFrameData() override
	{
		Super::DrawFrameData();
		ImGui::Text("DX11 frame time: %f", m_HwFrame);
//		m_hardwareRenderer->DrawControls();
	}

	void OnRenderStart() override {
		Super::OnRenderStart();
		m_Camera.Tick(m_FrameTime);
		m_HwTimer.Reset();
		m_hardwareRenderer->Render(scene);
		m_HwFrame = m_HwTimer.ElapsedMillis();
	}

	void Resize(u32 width, u32 height)
	{
		//m_ViewHeight = height;
		//m_ViewWidth = width;
		m_hardwareRenderer->Resize(width, height);
	}

	DX11Renderer* Renderer() override { return m_hardwareRenderer.get(); }

	std::unique_ptr<DX11Renderer> m_hardwareRenderer;
protected:
	Walnut::Timer m_HwTimer;
	float m_HwFrame = 0;
	//std::vector<ComPtr
};
