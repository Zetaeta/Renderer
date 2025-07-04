#include "RenderManagerDX11.h"
#include "editor/Viewport.h"
#include "render/RendererScene.h"
//
// Usage: SetThreadName (-1, "MainThread");
//
#include <windows.h>
#include "common/Config.h"
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
	DWORD  dwType;	   // Must be 0x1000.
	LPCSTR szName;	   // Pointer to name (in user addr space).
	DWORD  dwThreadID; // Thread ID (-1=caller thread).
	DWORD  dwFlags;	   // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, const char* threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

ConfigVariableAuto<int> CVarImguiBackend("imgui.backend", 11, "Backend for imgui rendering. 11 = DX11, 12 = DX12");

namespace  rnd::dx11
{
RenderManagerDX11::RenderManagerDX11(ID3D11Device* device, ID3D11DeviceContext* context, Input* input, bool createDx12 /*= false*/)
	: Super(input)
{
	m_hardwareRenderer = std::make_unique<DX11Renderer>(&mScene, &m_Camera, 0, 0, device, context);
	//auto& mWin11 = mWindows.emplace_back(std::make_unique<wnd::Window>(1500, 900, L"DX11"));
	//m_hardwareRenderer->CreateSurface(mWin11.get())->CreateFullscreenViewport(&mScene, &m_Camera);
	CreateIndependentViewport(m_hardwareRenderer.get(), L"DX11");
	if (CVarImguiBackend.GetValueOnMainThread() == 11)
	{
		m_hardwareRenderer->ImGuiInit(mWindows[0].get());
	}
	if (createDx12)
	{
		dx12Win = MakeOwning<dx12::DX12RHI>(1500, 900, L"DX12", TripleBuffered, &mScene, &m_Camera);
		dx12LOR = dx12Win->GetLiveObjectReporter();
		auto* window = CreateIndependentViewport(dx12Win.get(), L"DX12.1");
		if (CVarImguiBackend.GetValueOnMainThread() == 12)
		{
			dx12Win->ImGuiInit(window);
		}
	}
	RenderResourceMgr::OnDevicesReady();
	mRenderThread = std::thread([this]
	{
		SetThreadName(GetCurrentThreadId(), "Render thread");
		while (!mExitRequested.load(std::memory_order_acquire))
		{
			GRenderController.BeginRenderFrame();
			GRenderController.EndRenderFrame();
		}
	});
}

wnd::Window* RenderManagerDX11::CreateIndependentViewport(rnd::IRenderDevice* device, wchar_t const* name)
{
	auto& win = mWindows.emplace_back(std::make_unique<wnd::Window>(1500, 900, name));
	device->CreateSurface(win.get())->CreateFullscreenViewport(&mScene, new UserCamera(mInput, win.get()));
	return win.get();
}

void RenderManagerDX11::DrawFrameData()
{
	Super::DrawFrameData();
	ImGui::Text("DX11 frame time: %f", m_HwFrame);
	if (ImGui::Button("new dx11 window"))
	{
		CreateIndependentViewport(m_hardwareRenderer.get(), L"DX11");
	}
	if (ImGui::Button("new dx12 window"))
	{
		CreateIndependentViewport(dx12Win.get(), L"DX12");
	}
	//		m_hardwareRenderer->DrawControls();
}

RenderManagerDX11::~RenderManagerDX11()
{
	mExitRequested = true;
	GRenderController.RequestExit();
	mRenderThread.join();
}

void rnd::dx11::RenderManagerDX11::OnRenderStart()
{
//	uvec2 size = mWindows[0]->GetSize();
	//float scale = float(std::min(size.x, size.y));
	//m_Camera.SetViewExtent(.5f * size.x / scale, .5f * size.y / scale);
	Super::OnRenderStart();
//	m_Camera.Tick(m_FrameTime);
	Tickable::TickAll(m_FrameTime);
	m_HwTimer.Reset();
//	mScene.DataInterface().FlipBuffer_MainThread();
	BufferedRenderInterface::FlipBuffer_MainThread();
	if (m_hardwareRenderer->GetViewport() && !m_hardwareRenderer->GetViewport()->mRScene->IsInitialized())
	{
		m_hardwareRenderer->GetViewport()->mRScene = RendererScene::Get(mScene, m_hardwareRenderer.get());
	}

	//		m_hardwareRenderer->Render(mScene);
	//GRenderController.BeginRenderFrame();
	//GRenderController.EndRenderFrame();
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

}
