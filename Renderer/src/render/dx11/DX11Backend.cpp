#include "core/BaseDefines.h"
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>


#include "imgui.h"
#include "backends/imgui_impl_win32.cpp"
#include "backends/imgui_impl_dx11.cpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#include "render/dx11/DX11Backend.h"
#include "ImageRenderMgrDX11.h"
#include "core/WinUtils.h"
#include "RenderManagerDX11.h"
#include "core/Hash.h"
#include <editor/Editor.h>
#include "render/dx11/DX11Texture.h"
#include "core/Logging.h"
#include "render/dx12/DX12Window.h"
#include "common/ImguiThreading.h"
#include "common/Application.h"
#include "core/StringView.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
//#pragma comment(lib, "d3dx11.lib")

using namespace rnd;
using namespace rnd::dx11;

// Data
static ComPtr<ID3D11Device>		   g_pd3dDevice = nullptr;
static ID3D11DeviceContext*	   g_pContext = nullptr;
//static IDXGISwapChain*		   g_pSwapChain = nullptr;
static UINT					   g_ResizeWidth = 0, g_ResizeHeight = 0;
static INT					   g_WindowPosX = 0, g_WindowPosY = 0;
static UINT					   g_CurrWidth = 0, g_CurrHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

ComPtr<ID3D11Texture2D> g_msaaRenderTarget;
ComPtr<ID3D11RenderTargetView> g_msaaRenderTargetView;
	ComPtr<ID3D11Texture2D> g_BackBuffer;

// Forward declarations of helper functions
bool		   CreateDeviceD3D();
void		   CleanupDeviceD3D();
void		   CreateRenderTarget();
void		   CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


	struct Col
	{
		u8 r;
		u8 g;
		u8 b;
		u8 a;
	};
	struct Vert
	{
		
		vec4 pos;
		Col col;
	};

void DrawTri(vec4 a, vec4 b, vec4 c)
{

	Vert const		  vertices[] = {
		{a, {255,0,0, 1}}, {b,{0,255,0,1}}, {c,{0,0,255,1}}
	};
	ComPtr<ID3D11Buffer> vBuffer;

	{
		D3D11_BUFFER_DESC bd = {};
		Zero(bd);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertices);
		bd.StructureByteStride = sizeof(Vert);
		D3D11_SUBRESOURCE_DATA sd;
		Zero(sd);
		sd.pSysMem = &vertices;


		HR_ERR_CHECK(g_pd3dDevice->CreateBuffer(&bd,&sd, &vBuffer));
	}
	const UINT stride = sizeof(Vert);
	const UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, vBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	u16 const indices[] = {0,1,2};
	ComPtr<ID3D11Buffer> iBuffer;

	{
		D3D11_BUFFER_DESC bd = {};
		Zero(bd);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(indices);
		bd.StructureByteStride = sizeof(u16);
		D3D11_SUBRESOURCE_DATA sd;
		Zero(sd);
		sd.pSysMem = &indices;

		HR_ERR_CHECK(g_pd3dDevice->CreateBuffer(&bd, &sd, &iBuffer));
	}
	g_pContext->IASetIndexBuffer(iBuffer.Get(),DXGI_FORMAT_R16_UINT, 0);

	//g_pContext->Draw((UINT)std::size(vertices), 0u);

	g_pContext->DrawIndexed(Size(indices),0,0);

}
ComPtr<ID3D11DepthStencilView> dsv;

DEFINE_LOG_CATEGORY_STATIC(LogDX11Backend)

bool HasArg(const String& arg, int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (EqualsIgnoreCase(arg, argv[i]))
		{
			return true;
		}
	}

	return false;
}


int SetupConsole()
{
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hIn == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwOriginalOutMode = 0;
	DWORD dwOriginalInMode = 0;
	if (!GetConsoleMode(hOut, &dwOriginalOutMode))
	{
		return false;
	}
	if (!GetConsoleMode(hIn, &dwOriginalInMode))
	{
		return false;
	}

	DWORD dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
	DWORD dwRequestedInModes = ENABLE_VIRTUAL_TERMINAL_INPUT;

	DWORD dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
	if (!SetConsoleMode(hOut, dwOutMode))
	{
		// we failed to set both modes, try to step down mode gracefully.
		dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
		if (!SetConsoleMode(hOut, dwOutMode))
		{
			// Failed to set any VT mode, can't do anything here.
			return -1;
		}
	}

	DWORD dwInMode = dwOriginalInMode | dwRequestedInModes;
	if (!SetConsoleMode(hIn, dwInMode))
	{
		// Failed to set VT input mode, can't do anything here.
		return -1;
	}
	return 0;
}

// Main code
int MainDX11(int argc, char** argv)
{
	SetupConsole();
	LogConsumerThread logThread;
	logThread.Start();
	// Create application window
	// ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D())
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	//::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);
	

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;	  // Enable Multi-Viewport / Platform Windows
	// io.ConfigViewportsNoAutoMerge = true;
	// io.ConfigViewportsNoTaskBarIcon = true;
	// io.ConfigViewportsNoDefaultParent = true;
	// io.ConfigDockingAlwaysTabBar = true;
	// io.ConfigDockingTransparentPayload = true;
	// io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
	// io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	//ImGui_ImplWin32_Init(hwnd);
	//ImGui_ImplDX11_Init(g_pd3dDevice.Get(), g_pContext);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// io.Fonts->AddFontDefault();
	// io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	// ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	// IM_ASSERT(font != nullptr);

	// Our state
	bool   show_demo_window = false;
	bool   show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	InputImgui input;
	{

	RenderManagerDX11 renderMgr(g_pd3dDevice.Get(), g_pContext, &input, HasArg("-dx12", argc, argv));
	Editor*			  editor = Editor::Create(&input, &renderMgr);
	renderMgr.CreateInitialScene();
//	std::shared_ptr<rnd::dx11::DX11Texture> bbTex = nullptr;

	// Main loop
	bool done = false;
	while (!done && !App::IsExitRequested())
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			//CleanupRenderTarget();
			//g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_CurrWidth = g_ResizeWidth;
			g_CurrHeight = g_ResizeHeight;
			g_ResizeWidth = g_ResizeHeight = 0;
			//CreateRenderTarget();
			//D3D11_TEXTURE2D_DESC td;
			//g_BackBuffer->GetDesc(&td);
			//ZE_ASSERT (td.Width == g_CurrWidth && td.Height == g_CurrHeight);
			//DeviceTextureDesc desc;
			//desc.Width = g_CurrWidth;
			//desc.Height = g_CurrHeight;
			//desc.Flags = TF_RenderTarget;
			//bbTex = std::make_shared<rnd::dx11::DX11Texture>(renderMgr.m_hardwareRenderer->m_Ctx,desc, g_BackBuffer.Get());
			renderMgr.Resize(g_CurrWidth, g_CurrHeight);
//			renderMgr.m_hardwareRenderer->Resize(g_CurrWidth, g_CurrHeight);
			editor->OnWindowResize(g_CurrWidth, g_CurrHeight);

//			renderMgr.m_hardwareRenderer->SetBackbuffer(bbTex, g_CurrWidth, g_CurrHeight);
		}



		// Start the Dear ImGui frame
		//ImGui_ImplDX11_NewFrame();
		//ImGui_ImplWin32_NewFrame();
		//ImGui::NewFrame();
		ThreadImgui::BeginFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);



//a		g_pContext->OMSetRenderTargets(1, g_msaaRenderTargetView.GetAddressOf(), dsv.Get());
		//g_pContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		//const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		//g_pContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		//g_pContext->ClearRenderTargetView(g_msaaRenderTargetView.Get(), clear_color_with_alpha);
		//g_pContext->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		//if (g_CurrHeight && g_CurrWidth) {
			//renderMgr.m_hardwareRenderer->GetViewport()->UpdatePos({g_WindowPosX, g_WindowPosY});
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

			// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
			// because it would be confusing to have two docking targets within each others.
			/**/ ImGuiWindowFlags window_flags = 0;//ImGuiWindowFlags_NoDocking;
			//if (m_MenubarCallback)
			//	window_flags |= ImGuiWindowFlags_MenuBar;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			 ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMouseInputs;

			// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
			// and handle the pass-thru hole, so we ask Begin() to not render a background.
			//if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				window_flags |= ImGuiWindowFlags_NoBackground;

			// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
			// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
			// all active windows docked into it will lose their parent and become undocked.
			// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
			// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("My DockSpace", nullptr, window_flags);
			ImGui::PopStyleVar();

			ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				//ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
				//ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
				ImGui::DockSpaceOverViewport(0, viewport, dockspace_flags);
			}
			/* if (m_MenubarCallback)
			{
				if (ImGui::BeginMenuBar())
				{
					m_MenubarCallback();
					ImGui::EndMenuBar();
				}
			}*/

			ImGui::End();
			editor->Tick(0);
			renderMgr.DrawUI();

			static float f = 0.0f;
			static int	 counter = 0;

		//}
		ThreadImgui::EndFrame();

//		auto rt = bbTex->GetRT()->GetData<ID3D11RenderTargetView>();
//		g_pContext->OMSetRenderTargets(1, &rt, nullptr);
		// Rendering
//		renderMgr.m_hardwareRenderer->PreImgui();
		//ImGui::Render();
		//ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			//ImGui::UpdatePlatformWindows();
			//ImGui::RenderPlatformWindowsDefault();
		}

//		g_pSwapChain->Present(1, 0); // Present with vsync
									 // g_pSwapChain->Present(0, 0); // Present without vsync
	}
	Editor::Destroy();
	}

	// Cleanup
	ImGui::DestroyContext();

	CleanupDeviceD3D();

	logThread.Flush();
	logThread.RequestStop();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	logThread.Join();

	return 0;
}

// Helper functions

bool CreateDeviceD3D()
{
	// Setup swap chain

	UINT createDeviceFlags = 0;
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL		featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &g_pd3dDevice, &featureLevel, &g_pContext);
	DXCALL(res);
	//if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
	//	res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pContext);
	if (res != S_OK)
		return false;

	ComPtr<ID3D11Debug> debug;
	if (SUCCEEDED(g_pd3dDevice.As(&debug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(debug.As(&d3dInfoQueue)))
		{
			D3D11_MESSAGE_ID hide[] = {
				D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET
			};
			D3D11_INFO_QUEUE_FILTER filter;
			Zero(filter);
			filter.DenyList.NumIDs = NumCast<u32>(std::size(hide));
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
//			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
//			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
		}
	}

//	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	//if (g_pSwapChain)
	//{
	//	g_pSwapChain->Release();
	//	g_pSwapChain = nullptr;
	//}
	if (g_pContext)
	{
		g_pContext->Release();
		g_pContext = nullptr;
	}
	if (g_pd3dDevice)
	{
		g_pd3dDevice = nullptr;
	}
	{
		ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
}


void CleanupRenderTarget()
{
	g_BackBuffer = nullptr;
	return;
	if (g_mainRenderTargetView)
	{
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
				return 0;
			g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
			g_ResizeHeight = (UINT)HIWORD(lParam);
			return 0;
		case WM_MOVE:
			g_WindowPosX = (INT)(short)LOWORD(lParam); // Queue resize
			g_WindowPosY = (INT)(short)HIWORD(lParam);
			break;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
