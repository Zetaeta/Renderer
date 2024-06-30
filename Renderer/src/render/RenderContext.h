#pragma once
#include <scene/Camera.h>
#include "DeviceTexture.h"
#include <scene/Lights.h>
#include "RenderDevice.h"

struct Scene;
struct DX11Ctx;
class IRenderDeviceCtx;
namespace rnd { class RenderCubemap; }

namespace rnd { class IRenderDevice; }

namespace rnd
{

class RenderPass;
//template<typename T>
class LightRenderData
{
public:
	bool IsValid()
	{
		return mCamera != nullptr && mShadowMap != nullptr;
	}
	OwningPtr<Camera> mCamera;
	IDeviceTexture::Ref mShadowMap;

	Camera::ConstRef GetCamera() const;
	Camera::Ref GetCamera();
	//	u32 mLightIdx;
};

enum class EShadingDebugMode : u32
{
	NONE = 0,
	ALBEDO,
	NORMAL,
	ROUGHNESS,
	METALNESS,
	DEPTH,
	SHADOW_DEPTH,
	COUNT
};


class RenderContext
{
public:
	RenderContext(DX11Ctx* mCtx, Camera::Ref camera, IRenderTarget::Ref rt, IDepthStencil::Ref ds);
	void RenderFrame(Scene const& scene);

	LightRenderData CreateLightRenderData(ELightType lightType, u32 lightIdx);

	void SetupLightData();

	Vector<LightRenderData> const& GetLightData(ELightType lightType)
	{
		return mLightData[Denum(lightType)];
	}

	LightRenderData const& GetLightData(ELightType lightType, u32 lightIdx)
	{
		return GetLightData(lightType)[lightIdx];
	}

	IRenderDeviceCtx* DeviceCtx()
	{
		return mDeviceCtx;
	}

	const Scene& GetScene() { return *mScene; }

	DX11Ctx* mCtx;

	bool mLayersEnabled[Denum(EShadingLayer::COUNT)];

	EShadingDebugMode ShadingDebugMode = EShadingDebugMode::NONE;
	float mDebugGrayscaleExp = 1.f;
	float mDirShadowmapScale = 20;
	int mBrdf = 0;

	void SetDebugCube(IDeviceTextureCube* cubemap);
	void DrawControls();
	IDeviceTexture* mDebugCube = nullptr;

private:
	IRenderDevice* mDevice = nullptr;
	IRenderDeviceCtx* mDeviceCtx = nullptr;
	Scene const* mScene = nullptr;
	Vector<OwningPtr<RenderPass>> mPasses;
	RenderCubemap* mDebugCubePass;
	// Vector<LightRenderData> mSpotLightData;
	// Vector<LightRenderData> mDirLightData;
	Vector<LightRenderData> mLightData[Denum(ELightType::COUNT)];
};
} // namespace rnd
