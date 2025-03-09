#pragma once
#include "imgui.h"
#include "core/Async.h"
#include "thread/ThreadUtils.h"
#include "BufferedRenderInterface.h"
#include "container/HandledVector.h"

namespace ThreadImgui
{
class ImDrawDataWrapper : public ImDrawData
{
public:
	ImDrawDataWrapper() = default;
	ImDrawDataWrapper(ImDrawData* drawDataFromImgui);
	ImDrawDataWrapper(ImDrawDataWrapper&& other);
	ImDrawDataWrapper(ImDrawDataWrapper const& other);
	ImDrawDataWrapper& operator=(ImDrawDataWrapper&& other);
	ImDrawDataWrapper& operator=(ImDrawData* drawDataFromImgui);
	~ImDrawDataWrapper();

	void Reset()
	{
		ClearData();
	}

	private:
	void CopyData(ImDrawData const& other);
	void MoveData(ImDrawDataWrapper&& other);
	void ClearData();
	void CopyTrivial(ImDrawData const& other);
};

struct ImguiPlatformWindow : ImGuiViewport
{
	ImguiPlatformWindow() {}
	ImguiPlatformWindow(ImGuiViewport const& other);
	ImguiPlatformWindow& operator=(ImGuiViewport const& other);

	ImguiPlatformWindow(ImguiPlatformWindow&& other);
	ImguiPlatformWindow& operator=(ImguiPlatformWindow&& other);
	~ImguiPlatformWindow();

	void Clear();
};

struct ImguiDataForDrawing
{
	ImDrawDataWrapper DrawData;
	Vector<ImguiPlatformWindow> PlatformWindows;
};
class ImguiThreadInterface : public TBufferedRenderInterface<ImguiDataForDrawing>
{
public:
	static void Init();
private:
	//std::array<ImDrawDataWrapper, NumSceneDataBuffers> mDrawDatas;
	//std::array<Vector<ImguiPlatformWindow>, NumSceneDataBuffers> mPlatformWindows;

	void FlipFrameBuffers(u32 fromIndex, u32 toIndex) override;
};

void BeginFrame();
void EndFrame();
ImDrawDataWrapper const& GetDrawData_RenderThread();
Vector<ImguiPlatformWindow>& GetPlatformWindows_RenderThread();

using ImguiCallback = std::function<void()>;
void EnqueueImguiFunc(ImguiCallback&& callback, CancellationHandle&& cancelHdl);
void ExecuteQueuedFuncs();
void DeferredDestroyWindows();

HandledVec::Handle RegisterImguiFunc(std::function<void()> func, CancellationHandle cancelHdl = {});
bool UnregisterImguiFunc(HandledVec::Handle hdl);

}

inline bool IsInImguiThread()
{
	return IsInMainThread();
}

template<typename Func>
inline void RunImgui(Func&& func, CancellationHandle cancelHdl = {})
{
	if (IsInImguiThread() && false)
	{
		func();
	}
	else
	{
		ThreadImgui::EnqueueImguiFunc(func, std::move(cancelHdl));
	}
}
