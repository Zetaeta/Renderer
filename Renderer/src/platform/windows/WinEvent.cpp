#include "WinEvent.h"
#include "container/MovableObjectPool.h"
#include "WinApi.h"

#include <array>
#include "core/Utils.h"

namespace wnd
{

static std::array<MovableObjectPool<HANDLE>, 2> sEventPools;

WinEvent ClaimEvent(bool manualReset)
{
	WinEvent evt;
	HANDLE* handle;
	evt.mPoolId.ManualReset = manualReset;
	bool bNeedsInit = sEventPools[manualReset].Claim(evt.mPoolId.ItemID, handle);
	if (bNeedsInit)
	{
		*handle = CreateEventA(nullptr, manualReset, false, nullptr);
	}
	evt.mHandle = *handle;
	return evt;
} 

void ReleaseEvent(WinEvent evt)
{
	sEventPools[evt.mPoolId.ManualReset].Release(evt.mPoolId.ItemID);
}

bool WinEvent::Wait(u32 maxTimeMs) const
{
	DWORD result = WaitForSingleObject(mHandle, maxTimeMs);
	if (result == WAIT_TIMEOUT)
	{
		return false;
	}
	ZE_ASSERT(result == WAIT_OBJECT_0);
	return true;
}

}
