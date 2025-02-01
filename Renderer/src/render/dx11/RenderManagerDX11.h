#pragma once
#include "ImageRenderMgrDX11.h"
#include "render/dx12/DX12RHI.h"
#include "render/RenderController.h"

namespace rnd
{
namespace dx11
{

class RenderManagerDX11 : public RenderManager
{
public:
	using Super = RenderManager;

	RenderManagerDX11(ID3D11Device* device, ID3D11DeviceContext* context, Input* input, bool createDx12 = false);
	
	virtual void DrawFrameData() override
	{
		Super::DrawFrameData();
		ImGui::Text("DX11 frame time: %f", m_HwFrame);
//		m_hardwareRenderer->DrawControls();
	}

	void CreateIndependentViewport(rnd::IRenderDevice* device, wchar_t const* name);

	void OnRenderStart() override;

	void Resize(u32 width, u32 height)
	{
		//m_ViewHeight = height;
		//m_ViewWidth = width;
		m_hardwareRenderer->Resize(width, height);
	}

	virtual rnd::IRenderDeviceCtx* DeviceCtx() override;
	virtual rnd::IRenderDevice* Device() override;

	DX11Renderer* Renderer() override { return m_hardwareRenderer.get(); }

	std::unique_ptr<DX11Renderer> m_hardwareRenderer;
	Vector<std::unique_ptr<wnd::Window>> mWindows;
protected:
	Timer m_HwTimer;
	float m_HwFrame = 0;
	OwningPtr<dx12::DX12RHI> dx12Win = nullptr;
	OwningPtr<dx12::DX12RHI::LiveObjectReporter> dx12LOR;
	//std::vector<ComPtr
};

}
}
