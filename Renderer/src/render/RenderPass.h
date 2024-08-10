#pragma once


namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;

class RenderPass
{
public:
	RenderPass(RenderContext* rCtx)
		: mRCtx(rCtx)
	{
	}
	virtual ~RenderPass() {}
	virtual void RenderFrame(RenderContext& renderCtx) = 0;
	bool IsEnabled()
	{
		return mEnabled;
	}

	void SetEnabled(bool enabled)
	{
		mEnabled = enabled;
	}

	IRenderDeviceCtx* DeviceCtx();

protected:
	RenderContext* mRCtx = nullptr;
	bool mEnabled = true;
};


} // namespace rnd

