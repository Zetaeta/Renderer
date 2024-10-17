#pragma once

#include "dx11/SharedD3D11.h"
#include <scene/Camera.h>
#include <render/DeviceTexture.h>
#include <render/ConstantBuffer.h>
#include <common/Material.h>
#include "render/dx11/DX11Material.h"
#include "RenderContext.h"
#include "RenderDeviceCtx.h"
#include "RenderPass.h"
#include "scene/Lights.h"

struct MeshPart;
class SceneComponent;
struct DX11Ctx;
class Scene;

namespace rnd
{

struct DrawData
{
	MeshPart const* mesh;
	SceneComponent const* component;
};

enum class ERenderPassMode : u8
{
	IMMEDIATE,
	DEFERRED,
	ASYNC
};

using RenderDeviceCtx = DX11Ctx;

class SceneRenderPass : public RenderPass
{
public:
	SceneRenderPass(RenderContext* ctx, Name passName, Camera::Ref camera, IRenderTarget::Ref renderTarget, IDepthStencil::Ref depthStencil,
				EMatType accepts = E_MT_ALL, EShadingLayer layer = EShadingLayer::NONE, ERenderPassMode mode = ERenderPassMode::IMMEDIATE);

	bool Accepts(SceneComponent const* sceneComp, MeshPart const* mesh) const;

	virtual void Execute(RenderContext& renderCtx) override;
	void RenderScene(Scene const& scene);

	virtual void Accept(SceneComponent const* sceneComp, MeshPart const* mesh);

	virtual void BeginRender();
	virtual void OnCollectFinished() {}

	virtual void SetupShading();

	constexpr static bool SPLIT_DIRECTIONS = true;
	constexpr static u32 CUBE_FACES = 6;
	
	void SetCubeRTAndDS(u8 face);

	void RenderBuffer();
	void ProcessBuffer();

	void Draw(DrawData const& data);
	void DrawSingle(DrawData const& data, mat4 const& projection, mat4 const& projWorld);

	bool IsDepthOnly() { return mLayer == EShadingLayer::DEPTH; }
	bool IsDeferred() { return mMode == ERenderPassMode::DEFERRED; }

	IDeviceMaterial* mMatOverride = nullptr;

protected:
	Vector<DrawData> mBuffer;

	Name mPassName;
	Scene const* mScene;
	ERenderPassMode mMode;
	EMatType mAcceptedMatTypes;
	IRenderTarget::Ref mRenderTarget;
	IDepthStencil::Ref mDepthStencil;
	Camera::ConstRef mViewCam;
	EShadingLayer mLayer;
	bool mDepthOnly;

private:
};

class RenderSpotShadow : public SceneRenderPass
{
//	RenderSpotShadow(RenderDeviceCtx* ctx, )
};

}
