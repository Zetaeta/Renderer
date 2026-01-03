#pragma once
#include "render/Renderer.h"
#include <vector>
#include "scene/Scene.h"
#include <array>
#include "render/RastRenderer.h"
#include <d3d11_1.h>
#include "core/WinUtils.h"
#include "render/dx11/DX11Texture.h"
#include "render/dx11/DX11Cubemap.h"
#include "render/dx11/DX11Ctx.h"
#include "DX11ConstantBuffer.h"
#include "render/SceneRenderPass.h"
#include "render/ConstantBuffer.h"
#include "render/RenderDevice.h"
#include "render/RenderDeviceCtx.h"
#include "DX11Device.h"
#include "render/DeviceMesh.h"
#include "container/EnumArray.h"
#include "DX11CBPool.h"
#include "core/memory/GrowingImmobileObjectPool.h"
#include "DX11Timer.h"
#include "render/RenderMaterial.h"

class Viewport;
namespace rnd
{
namespace dx11
{
using namespace rnd;
using namespace rnd::dx11;

class DX11Renderer : public IRenderer, public rnd::IRenderDeviceCtx, public DX11Device
{
public:
	DX11Renderer(Scene* scene, UserCamera* camera, u32 width, u32 height);

	~DX11Renderer();

	void DrawControls();

	void Setup();

	void Render(const Scene& scene) override;

	void PrepareShadowMaps();

	virtual IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size /* = 0 */) override;

	DX11ConstantBuffer& GetPerFramePSCB() { return m_PSPerFrameBuff; }
	DX11ConstantBuffer& GetPerFrameVSCB() { return m_VSPerFrameBuff; }

	DX11ConstantBuffer& GetPerInstancePSCB() { return m_PSPerInstanceBuff; }
	DX11ConstantBuffer& GetPerInstanceVSCB() { return m_VSPerInstanceBuff; }
	
	void DrawTexture(DX11Texture* tex, ivec2 pos = ivec2(0), ivec2 size = ivec2(-1));
	void SetBackbuffer(DX11Texture::Ref backBufferTex, u32 width, u32 height);

	// Begin IRenderDeviceCtx overrides
	void SetRTAndDS(IRenderTarget::Ref rt, IDepthStencil::Ref ds, int RTArrayIdx = -1, int DSArrayIdx = -1) override;
	void SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds) override;
	void SetViewport(float width, float height, float TopLeftX /* = 0 */, float TopLeftY /* = 0 */) override;
	void SetDepthStencilMode(EDepthMode mode, StencilState stencil = {}) override;
	void SetDepthMode(EDepthMode mode) override;
	void SetStencilState(StencilState state) override;
	void SetBlendMode(EBlendState mode) override;
	void ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil /* = 0 */) override;
	void ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour) override;
	void ClearUAV(UnorderedAccessView uav, vec4 clearValues) override;
	void ClearUAV(UnorderedAccessView uav, uint4 clearValues) override;
	void SetConstantBuffers(EShaderType shader, Span<IConstantBuffer* const> buffers) override;
	void SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const> handles) override;
	void ResolveMultisampled(DeviceSubresource const& Dest, DeviceSubresource const& Src);
	void	 UpdateConstantBuffer(CBHandle handle, std::span<const byte> data);
	void SetUAVs(EShaderType shader, UnorderedAccessViews uavs, u32 startIdx /* = 0 */) override;
	void UnbindUAVs(EShaderType shader, u32 clearNum, u32 startIdx /* = 0 */);
	void SetSamplers(EShaderType shader, Span<SamplerHandle const> samplers, u32 startSlot = 0) override;
	void DispatchCompute(ComputeDispatch args) override;
	void ClearResourceBindings() override;
	void ClearResourceBindings(EShaderType ShaderStage) override;

	virtual void SetShaderResources(EShaderType shader, Span<ResourceView const> const srvs, u32 startIdx = 0) override;
	void SetPixelShader(PixelShader const* shader) override;
	void SetVertexShader(VertexShader const* shader) override;
	void SetComputeShader(ComputeShader const* shader) override;
	void Copy(DeviceResourceRef dst, DeviceResourceRef src) override;

	void Wait(GPUSyncPoint* syncPoint) override {}

	MappedResource Readback(DeviceResourceRef resource, u32 subresource, _Out_opt_ RefPtr<GPUSyncPoint>* completionSyncPoint) override;

	bool GetImmediateContext(std::function<void(IRenderDeviceCtx&)> callback) override
	{
		callback(*this);
		return true;
	}

	#if PROFILING
	GPUTimer* CreateTimer(const wchar_t* Name);
	void StartTimer(GPUTimer* timer);
	void StopTimer(GPUTimer* timer);
	#endif
	// End IRenderDeviceCtx overrides



	void Resize(u32 width, u32 height, u32* canvas = nullptr) override;

	void ImGuiInit(wnd::Window* mainWindow);
	void PreImgui();
	void ImguiBeginFrame();
	void ImguiEndFrame();
	bool mRenderingImgui = false;


	MappedResource MapResource(ID3D11Resource*, u32 subResource, ECpuAccessFlags flags);

	void RegisterTexture(DX11Texture* tex)
	{
		m_TextureRegistry.push_back(tex);
	}

	void UnregisterTexture(DX11Texture* tex);

	void RegisterTexture(DX11Cubemap* tex)
	{
		m_CubemapRegistry.push_back(tex);
	}

	void UnregisterTexture(DX11Cubemap* tex);

	RenderMaterial* GetDefaultMaterial(int matType);

	IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData) override;
	IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData) override;
	void				ExecuteCommand(std::function<void(IRenderDeviceCtx & )>&& command, char const* name);
	void				BeginFrame() override;
	void				EndFrame() override;


	EnumArray<EShaderType, u32> mMaxShaderResources = {};
	EnumArray<EShaderType, u32> mMaxUAVs = {};
	DX11Ctx m_Ctx;

	Viewport* GetViewport()
	{
		return mViewport.get();
	}

	void SetVertexLayout(VertAttDescHandle attDescHandle) override;
	void UpdateInputLayout();

	VertAttDescHandle mCurrVertexLayoutHdl = -1;
	VertexAttributeDesc const* mCurrVertexLayout = nullptr;
	RefPtr<VertexShader const> mCurrVertexShader = nullptr;


	void DrawMesh(IDeviceMesh const* mesh) override;
	IRenderDeviceCtx* GetPersistentCtx() override
	{
		return this;
	}
	char const* GetName() const override
	{
		return "D3D11";
	}

	bool IsFirstFrame() const { return mFrameNum <= 1; }

 private:
	DX11Texture::Ref PrepareTexture(Texture const& tex, bool sRGB = false);

	RenderContext* mRCtx = nullptr;

	void ProcessTimers();
	void StartFrameTimer();
	void EndFrameTimer();

protected:

	ivec2 mViewerSize = {500, 500};

	void CreateDepthStencilState(ComPtr<ID3D11DepthStencilState>& state, EDepthMode depth, StencilState stencil);

	UserCamera* m_Camera;
	//Scene* m_Scene;
#if PROFILING
	GrowingImmobileObjectPool<DX11Timer, 256, true> mTimerPool;
	ComPtr<ID3DUserDefinedAnnotation> mUserAnnotation;
	RefPtr<GPUTimer> mFrameTimer;
	std::array<ComPtr<ID3D11Query>, DX11Timer::BufferSize> DisjointQueries;
#endif
	ID3D11Device*		   pDevice;
	ID3D11DeviceContext*	   pContext;
	DX11CBPool mCBPool;

	DX11Texture::Ref m_EmptyTexture;

	bool mRefactor = true;

	DX11ConstantBuffer m_VSPerInstanceBuff;
	DX11ConstantBuffer m_PSPerInstanceBuff;

	DX11ConstantBuffer m_PSPerFrameBuff;
	DX11ConstantBuffer m_VSPerFrameBuff;

	ComPtr<ID3D11SamplerState> m_Sampler;
	ComPtr<ID3D11BlendState> m_BlendState;
	ComPtr<ID3D11BlendState> m_AlphaBlendState;
	ComPtr<ID3D11BlendState> m_AlphaLitBlendState;
	EnumArray<EDepthMode, EnumArray<EStencilMode, ComPtr<ID3D11DepthStencilState>>> m_DSStates;

	ComPtr<ID3D11SamplerState> m_ShadowSampler;


	std::shared_ptr<DX11RenderTarget> m_MainRenderTarget;
	std::shared_ptr<DX11DepthStencil> m_MainDepthStencil;

	Vector<DX11Texture*> m_TextureRegistry;
	Vector<DX11Cubemap*> m_CubemapRegistry;

	OwningPtr<Viewport> mViewport;

	ComPtr<ID3D11Buffer> m_2DVBuff;
	DX11ConstantBuffer m_VS2DCBuff;
	DX11ConstantBuffer m_PS2DCBuff;

	DX11Device* myDevice = nullptr;

	std::unordered_map<MeshPart const*, PerInstanceVSData> m_PIVS;

	DX11Texture* m_ViewerTex = nullptr;

	u64 mFrameNum = 0;

	float m_Scale;
	u32 m_Width;
	u32 m_Height;
	std::vector<u32> m_PixelWidth;
	bool m_Dirty = false;
	bool mInputLayoutDirty = true;

	u32 m_PixelScale = 500;
	float xoffset;
	float yoffset;
	mat4 m_Projection;

	Scene* m_Scene;

	bool m_TextureViewer = false;
	float m_TexViewExp = 1.f;

	StencilState mStencilState;
	EDepthMode mDepthMode = EDepthMode::Less;
	

};

using DX11DeviceCtx = DX11Renderer;

}
}
