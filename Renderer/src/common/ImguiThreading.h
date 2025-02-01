#pragma once
#include "imgui.h"

namespace ThreadImgui
{
class ImDrawDataWrapper : public ImDrawData
{
public:
	ImDrawDataWrapper() = default;
	ImDrawDataWrapper(ImDrawData* drawDataFromImgui);
	ImDrawDataWrapper(ImDrawDataWrapper&& other);
	ImDrawDataWrapper& operator=(ImDrawDataWrapper&& other);
	ImDrawDataWrapper& operator=(ImDrawData* drawDataFromImgui);
	~ImDrawDataWrapper();
private:
	void CopyData(ImDrawData const& other);
	void MoveData(ImDrawDataWrapper&& other);
	void ClearData();
	void CopyTrivial(ImDrawData const& other);

};

void BeginFrame();
void EndFrame();
ImDrawDataWrapper GetDrawData();
}


// TODO multithreading
template<typename Func>
inline void RunImgui(Func&& func)
{
	func();
}
