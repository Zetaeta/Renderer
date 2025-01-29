#pragma once
#include "render/DeviceSurface.h"

namespace rnd::dx12
{
class DX12SwapChain : public IDeviceSurface
{
	void Present() override;
	void RequestResize(uvec2 newSize) override;
};

}
