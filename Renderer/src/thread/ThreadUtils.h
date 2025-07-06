#pragma once
#include "core/CoreTypes.h"

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
