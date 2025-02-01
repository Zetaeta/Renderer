#include "ImguiThreading.h"
#include "backends/imgui_impl_win32.h"
#include "mutex"
#include "backends/imgui_impl_dx11.h"

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

ImDrawDataWrapper& ImDrawDataWrapper::operator=(ImDrawDataWrapper&& other)
{
	ClearData();
	MoveData(std::move(other));
	return *this;
}

ImDrawDataWrapper& ImDrawDataWrapper::operator=(ImDrawData* drawDataFromImgui)
{
	ClearData();
	CopyData(*drawDataFromImgui);
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
	TotalIdxCount = other.TotalIdxCount;
	DisplayPos = other.DisplayPos;
	DisplaySize = other.DisplaySize;
	FramebufferScale = other.FramebufferScale;
	OwnerViewport = other.OwnerViewport;
}

void BeginFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

static ImDrawDataWrapper sDrawData;
static std::mutex sDrawDataMtx;

void EndFrame()
{
	ImGui::End();
	ImGui::Render();
	{
		std::lock_guard lock(sDrawDataMtx);	
		sDrawData = ImGui::GetDrawData();
	}
}

ThreadImgui::ImDrawDataWrapper GetDrawData()
{
	std::lock_guard lock(sDrawDataMtx);	
	ImDrawDataWrapper result(&sDrawData);
	return result;
}

}
