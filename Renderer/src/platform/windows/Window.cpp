#include "Window.h"
#include "unordered_map"
#include "core/Utils.h"
#include "render/DeviceSurface.h"
#include "common/Application.h"
namespace wnd
{

static std::unordered_map<HWND, Window*> sHwndToWindow;

static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			return 0;
		}

		if (auto it = sHwndToWindow.find(hWnd); it != sHwndToWindow.end())
		{
			it->second->Resize_WndProc((u32) LOWORD(lParam), (u32) HIWORD(lParam));
			return 0;
		}
		break;
	case WM_MOVE:
		if (auto it = sHwndToWindow.find(hWnd); it != sHwndToWindow.end())
		{
			it->second->Move_WndProc((int) LOWORD(lParam), (int) HIWORD(lParam));
			return 0;
		}
		break;
	case WM_DESTROY:
		if (auto it = sHwndToWindow.find(hWnd); it != sHwndToWindow.end())
		{
			it->second->OnDestroy_WndProc();
		}
		::PostQuitMessage(0);
		break;
	case WM_QUIT:
		App::RequestExit();
		break;
	default:
		break;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

Window::Window(u32 width, u32 height, wchar_t const* name)
:mWidth(width), mHeight(height)
{
	static WNDCLASSEX wc = [] {
		WNDCLASSEX cls = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ZEWindow", nullptr };
		ZE_ASSERT(RegisterClassEx(&wc) != 0);
		return cls;

	}();
	mHwnd = ::CreateWindowW(wc.lpszClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, width, height, nullptr, nullptr, wc.hInstance, nullptr);
	ZE_ASSERT(mHwnd != 0);
	sHwndToWindow[mHwnd] = this;

	ShowWindow(mHwnd, SW_SHOWDEFAULT);
	::UpdateWindow(mHwnd);
}

Window::~Window()
{
	::DestroyWindow(mHwnd);
	sHwndToWindow.erase(mHwnd);
}

void Window::Tick()
{
	
}

void Window::Resize_WndProc(u32 resizeWidth, u32 resizeHeight)
{
	if (mSurface)
	{
		mSurface->RequestResize({resizeWidth, resizeHeight});
	}
}

void Window::Move_WndProc(int posX, int posY)
{
	if (mSurface)
	{
		mSurface->OnMoved({posX, posY});
	}
}

void Window::OnDestroy_WndProc()
{

}

}
