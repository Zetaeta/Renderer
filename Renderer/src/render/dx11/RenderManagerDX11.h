#pragma once
#include "ImageRenderMgrDX11.h"
#include "render/dx12/DX12RHI.h"

namespace rnd
{
namespace dx11
{

class RenderManagerDX11 : public RenderManager
{
public:
	using Super = RenderManager;


	RenderManagerDX11(ID3D11Device* device, ID3D11DeviceContext* context, Input* input, IDXGISwapChain* swapChain, bool createDx12 = false)
		: Super(input)
	{
		m_hardwareRenderer = std::make_unique<DX11Renderer>(&mScene, &m_Camera, 0, 0, device, context, swapChain);
		if (createDx12)
		{
			dx12Win = MakeOwning<dx12::DX12RHI>(1500, 900, L"DX12", TripleBuffered, &mScene, &m_Camera);
			dx12LOR = dx12Win->GetLiveObjectReporter();
		}
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
		mScene.DataInterface().FlipBuffer_MainThread();
		m_hardwareRenderer->Render(mScene);
		m_HwFrame = m_HwTimer.ElapsedMillis();
		if (dx12Win)
		{
			dx12Win->Tick();
		}
	}

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
protected:
	Timer m_HwTimer;
	float m_HwFrame = 0;
	OwningPtr<dx12::DX12RHI> dx12Win = nullptr;
	OwningPtr<dx12::DX12RHI::LiveObjectReporter> dx12LOR;
	//std::vector<ComPtr
};

}
}
