#pragma once


namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
class IRenderDevice;

class RenderPass
{
public:
	RenderPass(RenderContext* rCtx)
		: mRCtx(rCtx)
	{
	}
	virtual ~RenderPass() {}
	virtual void Execute(RenderContext& renderCtx) = 0;
	bool IsEnabled()
	{
		return mEnabled;
	}

	void SetEnabled(bool enabled)
	{
		mEnabled = enabled;
	}

	IRenderDeviceCtx* DeviceCtx();
	IRenderDevice* Device();

protected:
	RenderContext* mRCtx = nullptr;
	bool mEnabled = true;
};


} // namespace rnd

