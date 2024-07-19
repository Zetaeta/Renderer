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
	RenderCubemap(EFlatRenderMode mode, String DebugName = "", IDeviceTexture* texture = nullptr );
	virtual void RenderFrame(RenderContext& renderCtx) override;

	void SetCubemap(IDeviceTexture* cubemap);

	IDeviceTextureCube* mCubemap;
	EFlatRenderMode mMode;
};

}
