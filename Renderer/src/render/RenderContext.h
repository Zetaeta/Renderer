#pragma once
#include <scene/Camera.h>
#include "DeviceTexture.h"
#include <scene/Lights.h>
#include "RenderDevice.h"
#include "RenderDeviceCtx.h"
#include <scene/ScreenObject.h>
#include "render/ConstantBuffer.h"

template<>
struct std::hash<u32vec2>
{
	std::size_t operator()(const u32vec2& vec) const
	{
		return std::hash<u64>()(u64(vec.x) << 32 | u64(vec.y));
	}
};

#define DEBUG_MODE_ALBEDO 1
#define DEBUG_MODE_NORMAL 2
#define DEBUG_MODE_ROUGHNESS 3
#define DEBUG_MODE_METALNESS 4
#define DEBUG_MODE_DEPTH 5
#define DEBUG_MODE_STENCIL 6

class Scene;
struct DX11Ctx;
class StaticMeshComponent;
namespace rnd { class RenderCubemap; }

namespace rnd { class IRenderDevice; }

namespace rnd
{

namespace CB
{
#define CBENTRY(name)\
	static Name name = #name;
	CBENTRY(colour);
	CBENTRY(emissiveColour);
	CBENTRY(roughness);
	CBENTRY(metalness);
	CBENTRY(ambdiffspec);
	CBENTRY(useNormalMap);
	CBENTRY(useEmissiveMap);
	CBENTRY(useRoughnessMap);
	CBENTRY(alphaMask);
	CBENTRY(screenObjectId);
}


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
	RenderContext(IRenderDeviceCtx* DeviceCtx, Camera::Ref camera, IDeviceTexture::Ref target);
	~RenderContext();

	void SetupRenderTarget();
	void SetupPasses();
	void RenderFrame(Scene const& scene);

	void Postprocessing();

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

	MaterialManager* GetMaterialManager()
	{
		return mDeviceCtx->MatManager;
	}

	template<typename TPass, typename... Args>
	void RunSinglePass(Args... args)
	{
		MakeOwning<TPass>(this, std::forward<Args>(args)...)->RenderFrame(*this);
	}

	const Scene& GetScene() { return *mScene; }


	bool mLayersEnabled[Denum(EShadingLayer::COUNT)];

	EShadingDebugMode ShadingDebugMode = EShadingDebugMode::NONE;
	float mDebugGrayscaleExp = 1.f;
	float mDirShadowmapScale = 20;
	static int mBrdf;

	void SetDebugCube(IDeviceTextureCube* cubemap);
	void DrawControls();
	IDeviceTexture* mDebugCube = nullptr;

	void DrawPrimComp(const StaticMeshComponent* component, const IDeviceMaterial* matOverride = nullptr, EShadingLayer layer = EShadingLayer::BASE);
	void DrawPrimitive(const MeshPart* primitive, const mat4& transform, const mat4& viewMatrix, const IDeviceMaterial* matOverride = nullptr, EShadingLayer layer = EShadingLayer::BASE);
	void SetScreenObjId(ScreenObjectId id)
	{
		mCurrentId = id;
	}

	const Camera& GetCamera()
	{
		return *mCamera;
	}
	CBDataSource& GetCBOverrides()
	{
		return mCBOverrides;
	}
	const CBDataSource& GetCBOverrides() const
	{
		return mCBOverrides;
	}

	IRenderTarget::Ref GetMainRenderTarget()
	{
		return mMainRT;
	}

	IDepthStencil::Ref GetMainDepthStencil()
	{
		return mMainDS;
	}

	IDepthStencil::Ref GetTempDepthStencilFor(IRenderTarget::Ref);

private:

	bool mUseMSAA = true;
	u32 msaaSampleCount = 4;

	IDeviceTexture::Ref mMsaaTarget;
	IDeviceTexture::Ref mTarget;
	IRenderTarget::Ref mMainRT;
	IDepthStencil::Ref mMainDS;
	IDeviceTexture::Ref mDSTex;
	IRenderDevice* mDevice = nullptr;
	IRenderDeviceCtx* mDeviceCtx = nullptr;
	Scene const* mScene = nullptr;
	Vector<OwningPtr<RenderPass>> mPasses;
	RenderCubemap* mDebugCubePass;
	Camera::Ref mCamera = nullptr;
	// Vector<LightRenderData> mSpotLightData;
	// Vector<LightRenderData> mDirLightData;
	Vector<LightRenderData> mLightData[Denum(ELightType::COUNT)];
	ScreenObjectId mCurrentId = SO_NONE;
	CBDataSource mCBOverrides;
	std::unordered_map<u32vec2, IDepthStencil::Ref> mTempDepthStencils;
};

class ScopedCBOverride
{
	RCOPY_MOVE_PROTECT(ScopedCBOverride);
public:
	template<typename T>
	ScopedCBOverride(RenderContext& rCtx, const Name& name, const T& value)
		: mName(name), mCbOverrides(rCtx.GetCBOverrides())
	{
		mCbOverrides.Set(name, value);
	}

	~ScopedCBOverride()
	{
		mCbOverrides.Unset(mName);
	}
private:
	Name mName;
	CBDataSource& mCbOverrides;
};
} // namespace rnd
