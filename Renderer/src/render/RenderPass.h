#pragma once
#include "core/Types.h"


namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
class IRenderDevice;

class RGBuilder;

class RenderPass
{
public:
	RenderPass(RenderContext* rCtx, String&& name = "")
		: mRCtx(rCtx), PassName(std::move(name))
	{
	}
	virtual ~RenderPass() {}
	virtual void Execute(RenderContext& renderCtx) = 0;
	virtual void Build(RGBuilder& builder) {}
	bool IsEnabled() const
	{
		return mEnabled;
	}

	void SetEnabled(bool enabled)
	{
		mEnabled = enabled;
	}

	const String& GetName() const
	{
		return PassName;
	}

	IRenderDeviceCtx* DeviceCtx();
	IRenderDevice* Device();

protected:
	String PassName;
	RenderContext* mRCtx = nullptr;
	bool mEnabled = true;
};


} // namespace rnd

