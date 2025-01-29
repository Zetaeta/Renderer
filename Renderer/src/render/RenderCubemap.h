#pragma once
#include "RenderPass.h"
#include "DeviceTexture.h"

namespace rnd
{

enum class EFlatRenderMode
{
	FRONT,
	BACK
};
 
class RenderCubemap : public RenderPass
{
public:
	RenderCubemap(RenderContext* renderCtx, EFlatRenderMode mode, String DebugName = "", IDeviceTexture* texture = nullptr );
	virtual void Execute(IRenderDeviceCtx& deviceCtx) override;

	void SetCubemap(IDeviceTexture* cubemap);

	IDeviceTextureCube* mCubemap;
	EFlatRenderMode mMode;
};

}
