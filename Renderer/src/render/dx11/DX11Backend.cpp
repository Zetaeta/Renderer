#include <stdio.h>
#include <windows.h>
#include <windowsx.h>


#include "imgui.h"
#include "backends/imgui_impl_win32.cpp"
#include "backends/imgui_impl_dx11.cpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>

#include "render/dx11/DX11Backend.h"
#include "ImageRenderMgrDX11.h"
#include "core/WinUtils.h"
#include "RenderManagerDX11.h"
#include "core/Hash.h"
#include <editor/Editor.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")
//#pragma comment(lib, "d3dx11.lib")

using namespace rnd;
using namespace rnd::dx11;

// Data
static ComPtr<ID3D11Device>		   g_pd3dDevice = nullptr;
static ID3D11DeviceContext*	   g_pContext = nullptr;
static IDXGISwapChain*		   g_pSwapChain = nullptr;
static UINT					   g_ResizeWidth = 0, g_ResizeHeight = 0;
static UINT					   g_CurrWidth = 0, g_CurrHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

ComPtr<ID3D11Texture2D> g_msaaRenderTarget;
ComPtr<ID3D11RenderTargetView> g_msaaRenderTargetView;
	ComPtr<ID3D11Texture2D> g_BackBuffer;

// Forward declarations of helper functions
bool		   CreateDeviceD3D(HWND hWnd);
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



// Main code
int MainDX11(int argc, char** argv)
{
	// Create application window
	// ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
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
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice.Get(), g_pContext);

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
	bool   show_demo_window = true;
	bool   show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	InputImgui input;
	RenderManagerDX11 renderMgr(g_pd3dDevice.Get(), g_pContext, &input);
	Editor*			  editor = Editor::Create(&input, &renderMgr);
	renderMgr.CreateInitialScene();

	// Main loop
	bool done = false;
	while (!done)
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
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_CurrWidth = g_ResizeWidth;
			g_CurrHeight = g_ResizeHeight;
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
			renderMgr.Resize(g_CurrWidth, g_CurrHeight);
			editor->OnWindowResize(g_CurrWidth, g_CurrHeight);
		}

		renderMgr.m_hardwareRenderer->SetMainRenderTarget(g_msaaRenderTargetView, dsv, g_CurrWidth, g_CurrHeight);


		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);



		g_pContext->OMSetRenderTargets(1, g_msaaRenderTargetView.GetAddressOf(), dsv.Get());
		//g_pContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		g_pContext->ClearRenderTargetView(g_msaaRenderTargetView.Get(), clear_color_with_alpha);
		g_pContext->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		if (true) {
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
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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
			ImGui::Begin("DockSpace Demo", nullptr, window_flags);
			ImGui::PopStyleVar();

			ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				//ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
				//ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
				ImGui::DockSpaceOverViewport(viewport,dockspace_flags);
			}
			/* if (m_MenubarCallback)
			{
				if (ImGui::BeginMenuBar())
				{
					m_MenubarCallback();
					ImGui::EndMenuBar();
				}
			}*/

			renderMgr.DrawUI();
			editor->Tick(0);

			ImGui::End();
			static float f = 0.0f;
			static int	 counter = 0;

		}
		g_pContext->ResolveSubresource(g_BackBuffer.Get(), 0, g_msaaRenderTarget.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		g_pContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		// Rendering
		ImGui::Render();
		//DrawTri({ 0, 0, 0,1 }, { .5, 1, 0,1 }, {1,0,0,1});
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		g_pSwapChain->Present(1, 0); // Present with vsync
									 // g_pSwapChain->Present(0, 0); // Present without vsync
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	UINT createDeviceFlags = 0;
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL		featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pContext);
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
		}
	}

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}
	if (g_pContext)
	{
		g_pContext->Release();
		g_pContext = nullptr;
	}
	if (g_pd3dDevice)
	{
		g_pd3dDevice = nullptr;
	}
}


void CreateRenderTarget()
{
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&g_BackBuffer));
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	Zero(rtvDesc);
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	g_pd3dDevice->CreateRenderTargetView(g_BackBuffer.Get(), &rtvDesc, &g_mainRenderTargetView);

	if (g_CurrWidth > 0 && g_CurrHeight > 0)
	{
		CD3D11_TEXTURE2D_DESC msaaRTDesc(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
										g_CurrWidth, g_CurrHeight,
										1,
										1,
										D3D11_BIND_RENDER_TARGET,
										D3D11_USAGE_DEFAULT,
										0,
										4);
		HR_ERR_CHECK(g_pd3dDevice->CreateTexture2D(&msaaRTDesc, nullptr, &g_msaaRenderTarget))
		CD3D11_RENDER_TARGET_VIEW_DESC msaaRTVDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS, msaaRTDesc.Format);
		HR_ERR_CHECK(g_pd3dDevice->CreateRenderTargetView(g_msaaRenderTarget.Get(), &msaaRTVDesc, &g_msaaRenderTargetView))

		D3D11_DEPTH_STENCIL_DESC dsDesc;
		Zero(dsDesc);
		dsDesc.DepthEnable = TRUE;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		ComPtr<ID3D11DepthStencilState> pDSState;
		HR_ERR_CHECK(g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSState));
		g_pContext->OMSetDepthStencilState(pDSState.Get(),0);

		ComPtr<ID3D11Texture2D> pDepthStencil;
		D3D11_TEXTURE2D_DESC depthDesc;
		Zero(depthDesc);
		depthDesc.Width = g_CurrWidth;
		depthDesc.Height = g_CurrHeight;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 4;
		depthDesc.SampleDesc.Quality = 0;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		HR_ERR_CHECK(g_pd3dDevice->CreateTexture2D(&depthDesc, nullptr, &pDepthStencil));

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		Zero(dsvDesc);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		dsvDesc.Texture2D.MipSlice = 0;
		HR_ERR_CHECK(g_pd3dDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &dsv));
	}
}

void CleanupRenderTarget()
{
	g_BackBuffer = nullptr;
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
