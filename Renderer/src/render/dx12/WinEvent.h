#pragma once

using HANDLE = void*;

namespace wnd
{

class WinEvent
{
private:
	HANDLE mHandle;
};

WinEvent ClaimEvent(bool manualReset);

wnd::WinEvent ClaimEvent(bool manualReset)
{

}

}
