#include "RenderManagerDX11.h"
#include "editor/Viewport.h"
#include "render/RendererScene.h"

void rnd::dx11::RenderManagerDX11::OnRenderStart()
{
	Super::OnRenderStart();
	m_Camera.Tick(m_FrameTime);
	m_HwTimer.Reset();
	mScene.DataInterface().FlipBuffer_MainThread();
	if (!m_hardwareRenderer->GetViewport()->mRScene->IsInitialized())
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
