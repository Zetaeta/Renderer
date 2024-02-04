#pragma once
#include "src/ImageRenderMgrDX11.h"
class RenderManagerDX11 : public ImageRenderMgrDX11
{
public:
	using Super = ImageRenderMgrDX11;


	RenderManagerDX11(ID3D11Device* device, ID3D11DeviceContext* context)
		: Super(device)
	{
		m_hardwareRenderer = std::make_unique<DX11Renderer>(&m_Camera, m_ViewWidth, m_ViewHeight, device, context);
	}
	
	virtual void DrawFrameData() override
	{
		Super::DrawFrameData();
		ImGui::Text("DX11 frame time: %f", m_HwFrame);
	}

	void OnRenderStart() override {
		Super::OnRenderStart();
		m_HwTimer.Reset();
		m_hardwareRenderer->Render(scene);
		m_HwFrame = m_HwTimer.ElapsedMillis();
	}

	void Resize(u32 width, u32 height)
	{
		m_hardwareRenderer->Resize(width, height);
	}

	std::unique_ptr<DX11Renderer> m_hardwareRenderer;
protected:
	Walnut::Timer m_HwTimer;
	float m_HwFrame = 0;
	//std::vector<ComPtr
};
