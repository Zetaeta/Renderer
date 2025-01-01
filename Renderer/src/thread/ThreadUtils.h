#pragma once

enum class ENamedThread : u8
{
	MainThread,
	RenderThread,
	Count

};


bool IsInMainThread();
bool IsInRenderThread();

namespace Thread
{
void SetNamedThread(ENamedThread thisThread);
}
