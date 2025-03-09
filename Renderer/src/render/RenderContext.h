#pragma once
#include <scene/Camera.h>
#include "DeviceTexture.h"
#include <scene/Lights.h>
#include "RenderDevice.h"
#include "RenderDeviceCtx.h"
#include <scene/ScreenObject.h>
#include "render/ConstantBuffer.h"
#include "ShaderManager.h"
#include "RenderGraph.h"
#include "core/Async.h"
#include "container/HandledVector.h"

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
namespace rnd { class RendererScene; }
namespace rnd { class RenderCubemap; }

namespace rnd { class IRenderDevice; }

namespace rnd
{

__declspec(align(16))
struct PerFrameVertexData
{
	DECLARE_STI_NOBASE(PerFrameVertexData);
	mat4 screen2World;
	mat4 world2Light;
	vec3 cameraPos;
};
DECLARE_CLASS_TYPEINFO(PerFrameVertexData);


struct PixelDebuggingSwitch : SHADER_PERM_BOOL("PIXEL_DEBUGGING");

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


struct RenderSettings
{
	RenderSettings();

	bool LayersEnabled[Denum(EShadingLayer::ForwardRenderCount)];
	EShadingDebugMode ShadingDebugMode = EShadingDebugMode::NONE;
	bool AmbientOcclusion = true;
	float DebugGrayscaleExp = 1.f;
	float DirShadowmapScale = 20;
	int2 DebugPixel {0};
	bool EnablePixelDebug = false;
};
extern RenderSettings  GSettings;

class RenderContext
{
public:
	RenderContext(IRenderDeviceCtx* DeviceCtx, RendererScene* scene, Camera::Ref camera, IDeviceTexture::Ref target, RenderSettings const& settings = {});
	~RenderContext();

	void SetupRenderTarget();
	void SetupPasses();
	void RenderFrame(rnd::IRenderDeviceCtx& ctx);

	void SetTarget(IDeviceTexture::Ref newTarget);

	RGBuilder& GraphBuilder()
	{
		return mRGBuilder;
	}

	void BuildGraph();
	IDeviceTexture::Ref GetPrimaryRT();

	void Postprocessing(rnd::IRenderDeviceCtx& ctx);

	template<typename T>
	T const* GetShader(T::Permutation const& permutation = {})
	{
		return GetShaderManager().GetCompiledShader<T>(T::sRegistryId, permutation);
	}

	LightRenderData CreateLightRenderData(ELightType lightType, u32 lightIdx);

	void SetupLightData();

	Vector<LightRenderData> const& GetLightData(ELightType lightType) const
	{
		return mLightData[Denum(lightType)];
	}

	LightRenderData const& GetLightData(ELightType lightType, u32 lightIdx) const
	{
		return GetLightData(lightType)[lightIdx];
	}

	IRenderDeviceCtx* DeviceCtx() const
	{
		return mDeviceCtx;
	}
	IRenderDevice* GetDevice()
	{
		return mDevice;
	}

	MaterialManager* GetMaterialManager()
	{
		return &mDeviceCtx->Device->MatMgr;
	}

	ICBPool* GetCBPool()
	{
		return DeviceCtx()->CBPool;
	}

	template<typename TPass, typename... Args>
	void RunSinglePass(IRenderDeviceCtx& ctx, Args&&... args)
	{
		MakeOwning<TPass>(this, std::forward<Args>(args)...)->ExecuteWithProfiling(ctx);
	}

	template<typename TPass, typename... Args>
	void AddPass(Args&&... args)
	{
		mPasses.emplace_back(MakeOwning<TPass>(this, std::forward<Args>(args)...));
	}

	const RendererScene& GetScene() const { return *mScene; }

	uint2 GetPrimaryRenderSize() const;

	RenderSettings& Settings = GSettings;

	static int mBrdf;

	void SetDebugCube(IDeviceTextureCube* cubemap);
	void DrawControls();
	IDeviceTexture* mDebugCube = nullptr;

	void DrawPrimComp(const StaticMeshComponent* component, const IDeviceMaterial* matOverride = nullptr, EShadingLayer layer = EShadingLayer::BASE);
	void DrawPrimitive(const IDeviceIndexedMesh* mesh, const mat4& transform, const mat4& viewMatrix, RenderMaterial* matOverride = nullptr, EShadingLayer layer = EShadingLayer::BASE);
	void SetScreenObjId(ScreenObjectId id)
	{
		mCurrentId = id;
	}

	void SetupPostProcess();

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

	ShaderManager& GetShaderManager()
	{
		return mDevice->ShaderMgr;
	}

	void DebugPixel(uint2 pixCoord)
	{
		Settings.DebugPixel = pixCoord;
	}

	UnorderedAccessView GetPixelDebugUav()
	{
		return mPixDebugUav;
	}

	void ShowPixelDebug(rnd::IRenderDeviceCtx& ctx);

	RenderTextureManager TextureManager;
private:

	Vector<IDeviceTexture::Ref> tempRemember;

	bool mUseMSAA = false;
	u32 msaaSampleCount = 4;

	IDeviceTexture::Ref mMsaaTarget;
	IDeviceTexture::Ref mPrimaryTarget;
	IDeviceTexture::Ref mTarget;
	IDeviceTexture::Ref mPPTarget;
	IDeviceTexture::Ref mBGCube;
	IRenderTarget::Ref mMainRT;
	IDepthStencil::Ref mMainDS;
	IDeviceTexture::Ref mDSTex;
	IRenderDevice* mDevice = nullptr;
	IRenderDeviceCtx* mDeviceCtx = nullptr;
	RendererScene* mScene = nullptr;
	Vector<OwningPtr<RenderPass>> mPasses;
	RenderCubemap* mDebugCubePass;
	Vector<OwningPtr<RenderPass>> mPPPasses;
	Camera::Ref mCamera = nullptr;
	// Vector<LightRenderData> mSpotLightData;
	// Vector<LightRenderData> mDirLightData;
	Vector<LightRenderData> mLightData[Denum(ELightType::COUNT)];
	ScreenObjectId mCurrentId = SO_NONE;
	CBDataSource mCBOverrides;
	std::unordered_map<u32vec2, IDepthStencil::Ref> mTempDepthStencils;

	RGBuilder mRGBuilder;
	RGResourceHandle mPingPongHandle; 
	RGResourceHandle mPixelDebugTex;
	UnorderedAccessView mPixDebugUav;
	DestructionToken mDestructionToken;
	HandledVec::Handle mImguiHandle{};
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
