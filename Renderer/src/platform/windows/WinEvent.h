#pragma once

#include "core/Maths.h"

using HANDLE = void*;

namespace wnd
{

class WinEvent
{
public:
	constexpr static u32 Infinite = (u32) -1;
	bool Wait(u32 maxTimeMs = Infinite) const;
	HANDLE GetHandle() const
	{
		return mHandle;
	}
private:
	HANDLE mHandle;
	struct 
	{
		u32 ItemID;
		bool ManualReset;
	} mPoolId;
	friend WinEvent ClaimEvent(bool manualReset);
	friend void ReleaseEvent(WinEvent event);
};

WinEvent ClaimEvent(bool manualReset);

void ReleaseEvent(WinEvent evt);



}
