#pragma once
#include "core/Types.h"
#include "GPUTimer.h"


namespace rnd
{
class RenderContext;
class IRenderDeviceCtx;
class IRenderDevice;

class RGBuilder;

class RenderPass
{
public:
	RenderPass(RenderContext* rCtx, HashString name = "");
	virtual ~RenderPass() {}

	void ExecuteWithProfiling(IRenderDeviceCtx& deviceCtx)
#if PROFILING
	;
#else
	{
		Execute(renderCtx);
	}
#endif
	virtual void Execute(IRenderDeviceCtx& deviceCtx) = 0;
	virtual void Build(RGBuilder& builder) {}
	bool IsEnabled() const
	{
		return mEnabled;
	}

	void SetEnabled(bool enabled)
	{
		mEnabled = enabled;
	}

	String GetName() const
	{
		return PassName.ToString();
	}

	void SetName(HashString name)
	{
		PassName = name;
	}

	IRenderDeviceCtx* DeviceCtx();
	IRenderDevice* Device();

	#if ZE_BUILD_EDITOR
	virtual void AddControls() {}
	#endif

#if PROFILING
	GPUTimer const* GetTimer() const
	{
		return mTimer;
	}
#endif

protected:
	HashString PassName;
	RenderContext* mRCtx = nullptr;
	IRenderDeviceCtx* mDeviceCtx = nullptr;
	bool mEnabled = true;
#if PROFILING
	RefPtr<GPUTimer> mTimer;
#endif
};

class LambdaPass : public RenderPass
{
public:
	LambdaPass(RenderContext* rCtx, std::function<void(IRenderDeviceCtx&)>&& lambda, HashString name = "")
		:RenderPass(rCtx, name), mLambda(lambda)
	{
	}
	void Execute(IRenderDeviceCtx& ctx) override
	{
		mLambda(ctx);
	}

private:
	std::function<void(IRenderDeviceCtx&)> mLambda;
};


} // namespace rnd

