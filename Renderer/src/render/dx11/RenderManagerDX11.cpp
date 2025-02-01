#include "RenderManagerDX11.h"
#include "editor/Viewport.h"
#include "render/RendererScene.h"

 rnd::dx11::RenderManagerDX11::RenderManagerDX11(ID3D11Device* device, ID3D11DeviceContext* context, Input* input, bool createDx12 /*= false*/)
	: Super(input)
{
	m_hardwareRenderer = std::make_unique<DX11Renderer>(&mScene, &m_Camera, 0, 0, device, context);
	mWin11 = std::make_unique<wnd::Window>(1500, 900, L"DX11");
	m_hardwareRenderer->CreateSurface(mWin11.get())->CreateFullscreenViewport(&mScene, &m_Camera);
	m_hardwareRenderer->ImGuiInit(mWin11.get());
	if (createDx12)
	{
		dx12Win = MakeOwning<dx12::DX12RHI>(1500, 900, L"DX12", TripleBuffered, &mScene, &m_Camera);
		dx12LOR = dx12Win->GetLiveObjectReporter();
	}
}

void rnd::dx11::RenderManagerDX11::OnRenderStart()
{
	uvec2 size = mWin11->GetSize();
	float scale = float(std::min(size.x, size.y));
	m_Camera.SetViewExtent(.5f * size.x / scale, .5f * size.y / scale);
	Super::OnRenderStart();
	m_Camera.Tick(m_FrameTime);
	m_HwTimer.Reset();
	mScene.DataInterface().FlipBuffer_MainThread();
	if (m_hardwareRenderer->GetViewport() && !m_hardwareRenderer->GetViewport()->mRScene->IsInitialized())
	{
		m_hardwareRenderer->GetViewport()->mRScene = RendererScene::Get(mScene, m_hardwareRenderer.get());
	}

	//		m_hardwareRenderer->Render(mScene);
	GRenderController.BeginRenderFrame();
	GRenderController.EndRenderFrame();
	m_HwFrame = m_HwTimer.ElapsedMillis();
	// if (dx12Win)
	//{
	//	dx12Win->Tick();
	// }
}

rnd::IRenderDeviceCtx* rnd::dx11::RenderManagerDX11::DeviceCtx()
{
	return Renderer();
}

rnd::IRenderDevice* rnd::dx11::RenderManagerDX11::Device()
{
	return Renderer();
}
