#include "ImguiThreading.h"
#include "mutex"
#include "thread/MPSCMessageQueue.h"
#include "backends/imgui_impl_win32.h"

namespace ThreadImgui
{

ImDrawDataWrapper::ImDrawDataWrapper(ImDrawData* drawDataFromImgui)
{
	CopyData(*drawDataFromImgui);
}

ImDrawDataWrapper::ImDrawDataWrapper(ImDrawDataWrapper&& other)
{
	MoveData(std::move(other));
}

ImDrawDataWrapper::ImDrawDataWrapper(ImDrawDataWrapper const& other)
{
	CopyData(other);
}

ImDrawDataWrapper& ImDrawDataWrapper::operator=(ImDrawDataWrapper&& other)
{
	ClearData();
	MoveData(std::move(other));
	return *this;
}

ImDrawDataWrapper& ImDrawDataWrapper::operator=(ImDrawData* drawDataFromImgui)
{
	ClearData();
	if (drawDataFromImgui)
	{
		CopyData(*drawDataFromImgui);
	}
	return *this;
}

ImDrawDataWrapper::~ImDrawDataWrapper()
{
	ClearData();
}

void ImDrawDataWrapper::CopyData(ImDrawData const& other)
{
	CopyTrivial(other);
	for (int i=0; i<CmdListsCount; ++i)
	{
		CmdLists.push_back(other.CmdLists[i]->CloneOutput());
	}
}

void ImDrawDataWrapper::MoveData(ImDrawDataWrapper&& other)
{
	CopyTrivial(other);
	CmdLists = std::move(other.CmdLists);
	other.CmdLists.clear();
}

void ImDrawDataWrapper::ClearData()
{
	CmdLists.clear_delete();
}

void ImDrawDataWrapper::CopyTrivial(ImDrawData const& other)
{
	Valid = other.Valid;
	CmdListsCount = other.CmdListsCount;
	TotalIdxCount = other.TotalIdxCount;
	TotalVtxCount = other.TotalVtxCount;
	DisplayPos = other.DisplayPos;
	DisplaySize = other.DisplaySize;
	FramebufferScale = other.FramebufferScale;
	OwnerViewport = other.OwnerViewport;
}

void BeginFrame()
{
	//ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

static ImDrawDataWrapper sDrawData;
static std::mutex sDrawDataMtx;

static ImguiThreadInterface GImguiThreadInterface;
void EndFrame()
{
	ExecuteQueuedFuncs();
	ImGui::Render();
	auto& threadData = GImguiThreadInterface.GetData_MainThread();
	threadData.DrawData = ImGui::GetDrawData();

	if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
	{
		ImGui::UpdatePlatformWindows();
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		threadData.PlatformWindows.clear();
		threadData.PlatformWindows.reserve(platformIO.Viewports.Size - 1);
		for (int i = 1; i < platformIO.Viewports.size(); ++i)
		{
			threadData.PlatformWindows.emplace_back(*platformIO.Viewports[i]);
		}
	}
}

ThreadImgui::ImDrawDataWrapper const& GetDrawData_RenderThread()
{
	return GImguiThreadInterface.GetData_RenderThread().DrawData;
}

Vector<ImguiPlatformWindow>& GetPlatformWindows_RenderThread()
{
	return const_cast<Vector<ImguiPlatformWindow>&>(GImguiThreadInterface.GetData_RenderThread().PlatformWindows);
}

struct ImguiQueueEntry
{
	ImguiCallback Callback;
	CancellationHandle CancelHdl;
};
static MPSCMessageQueue<ImguiQueueEntry> sImguiQueue;

void EnqueueImguiFunc(ImguiCallback&& callback, CancellationHandle&& cancelHdl)
{
	sImguiQueue.Add(ImguiQueueEntry{callback, cancelHdl});
}

static HandledVector<ImguiQueueEntry> gRegisteredFuncs;
static RWLock gRegisteredFuncsLock;

void ExecuteQueuedFuncs()
{
	sImguiQueue.ConsumeAll([](const ImguiQueueEntry& entry) {
		ReadLock guard = entry.CancelHdl ? entry.CancelHdl->GetCancelGuard() : ReadLock{};
		entry.Callback();
	}, false);

	auto lock = gRegisteredFuncsLock.ScopedReadLock();
	gRegisteredFuncs.ForEach([](const ImguiQueueEntry& entry) { entry.Callback(); });
}

HandledVec::Handle RegisterImguiFunc(std::function<void()> func, CancellationHandle cancelHdl /*= {}*/)
{
	auto lock = gRegisteredFuncsLock.ScopedWriteLock();
	return gRegisteredFuncs.Emplace(ImguiQueueEntry{func, cancelHdl});
}

bool UnregisterImguiFunc(HandledVec::Handle hdl)
{
	auto lock = gRegisteredFuncsLock.ScopedWriteLock();
	return gRegisteredFuncs.Remove(hdl);
}

static Vector<ImguiPlatformWindow> gDeferredDestroyWindows;

void RenderPlatformWindows()
{
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
	if (platformIO.Renderer_RenderWindow)
	{
		for (auto& Window : GetPlatformWindows_RenderThread())
		{
			if (Window.RendererUserData != nullptr && Window.DrawData != nullptr)
			{
				auto*& referencedRUD = Window.DrawData->OwnerViewport->RendererUserData;
				if (referencedRUD != Window.RendererUserData)
				{
					assert(referencedRUD == nullptr);
				}
				auto* oldRUD = referencedRUD;
				referencedRUD = Window.RendererUserData;

				platformIO.Renderer_RenderWindow(&Window, nullptr);
				if (platformIO.Renderer_SwapBuffers) platformIO.Renderer_SwapBuffers(&Window, nullptr);
				referencedRUD = oldRUD;
			}
		}
	}
	ThreadImgui::DeferredDestroyWindows();
}

static std::mutex gDeferredDestroyWindowsMtx;
static void ThreadImGui_DestroyWindow(ImGuiViewport* viewport)
{
	std::scoped_lock lock {gDeferredDestroyWindowsMtx};
	gDeferredDestroyWindows.emplace_back(*viewport);
	viewport->RendererUserData = nullptr;
}

static void (*sImGui_Platform_DestroyWindow)(ImGuiViewport* vp);
void ImguiThreadInterface::Init()
{
	sImGui_Platform_DestroyWindow = ImGui::GetPlatformIO().Renderer_DestroyWindow;
	ImGui::GetPlatformIO().Renderer_DestroyWindow = ThreadImGui_DestroyWindow;
}

void DeferredDestroyWindows()
{
	std::scoped_lock lock {gDeferredDestroyWindowsMtx};
	for (auto& window : gDeferredDestroyWindows)
	{
		sImGui_Platform_DestroyWindow(&window);
	}
	gDeferredDestroyWindows.clear();
}


void ImguiThreadInterface::FlipFrameBuffers(u32 fromIndex, u32 toIndex)
{
	mData[toIndex] = {};

}

ImguiPlatformWindow::ImguiPlatformWindow(ImGuiViewport const& other)
{
	memcpy(this, &other, sizeof(ImGuiViewport));
	if (other.DrawData)
	{
		DrawData = new ImDrawDataWrapper(other.DrawData);
	}
}

ImguiPlatformWindow::ImguiPlatformWindow(ImguiPlatformWindow&& other)
{
	memcpy(this, &other, sizeof(ImGuiViewport));
	other.DrawData = nullptr;
}

ImguiPlatformWindow& ImguiPlatformWindow::operator=(ImguiPlatformWindow&& other)
{
	Clear();
	memcpy(this, &other, sizeof(ImGuiViewport));
	other.DrawData = nullptr;
	return *this;
}

ImguiPlatformWindow& ImguiPlatformWindow::operator=(ImGuiViewport const& other)
{
	Clear();
	memcpy(this, &other, sizeof(ImGuiViewport));
	DrawData = new ImDrawDataWrapper(other.DrawData);
	return *this;
}

ImguiPlatformWindow::~ImguiPlatformWindow()
{
	Clear();
}

void ImguiPlatformWindow::Clear()
{
	if (DrawData)
	{
		delete static_cast<ImDrawDataWrapper*>(DrawData);
		DrawData = nullptr;
	}
	memset(this, 0, sizeof(ImguiPlatformWindow));
}

}
