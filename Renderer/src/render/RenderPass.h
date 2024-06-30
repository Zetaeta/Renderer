#pragma once

namespace rnd
{
class RenderContext;

class RenderPass
{
public:
	virtual void RenderFrame(RenderContext& renderCtx) = 0;
	bool IsEnabled()
	{
		return mEnabled;
	}

	void SetEnabled(bool enabled)
	{
		mEnabled = enabled;
	}
private:
	bool mEnabled = true;
};
} // namespace rnd

