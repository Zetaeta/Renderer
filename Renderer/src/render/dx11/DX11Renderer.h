#pragma once
#include "render/Renderer.h"
#include <vector>
#include "scene/Scene.h"
#include <array>
#include "render/RastRenderer.h"
#include <d3d11.h>
#include "core/WinUtils.h"
#include "render/dx11/DX11Texture.h"
#include "render/dx11/DX11Material.h"
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

class Viewport;
namespace rnd
{
namespace dx11
{
using namespace rnd;
using namespace rnd::dx11;

using col = vec3; 

struct PerInstanceVSData
{
	DECLARE_STI_NOBASE(PerInstanceVSData);
public:
	mat4 fullTransform;
	mat4 model2ShadeSpace;
	mat4 model2ShadeDual;
};
DECLARE_CLASS_TYPEINFO(PerInstanceVSData);

__declspec(align(16))
struct PerFramePSData
{
	col ambient;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
};

__declspec(align(16))
struct PFPSDirLight// : PerFramePSData
{
	col directionalCol;
	float _pad2;
	vec3 directionalDir;
	float _pad3;
	mat4 world2Light;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
};

__declspec(align(16))
struct PFPSPointLight// : PerFramePSData
{
	vec3 pointLightPos;
	float pointLightRad;
	vec3 pointLightCol;
	float pointLightFalloff;
	mat4 world2Light;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
};

__declspec(align(16))
struct PFPSSpotLight// : PerFramePSData
{
	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
	float _pad;
	mat4 world2Light;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
};

class DX11Renderer : public IRenderer, public rnd::IRenderDeviceCtx, public DX11Device
{
public:
	DX11Renderer(Scene* scene, UserCamera* camera, u32 width, u32 height, ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);

	~DX11Renderer();

	void DrawControls();

	void Setup();

	void Render(const Scene& scene) override;
	void Render(const Scene& scene, EShadingLayer layer, int index=-1, bool translucent = false);

	void LoadShaders(bool reload = false);

	void PrepareBG();
	void DrawBG();
	void DrawCubemap(ID3D11ShaderResourceView* srv, bool depth = false);
	void DrawCubemap(IDeviceTextureCube* cubemap);

	//template<typename T>
	//void CreateConstantBuffer(ComPtr<ID3D11Buffer>& buffer, T const& initialData = T {}, u32 size = sizeof(T));

	//template<typename T>
	//void WriteCBuffer(ComPtr<ID3D11Buffer>& buffer, T const& data);

	void PrepareShadowMaps();

	void DrawMesh(MeshPart const& meshPart, EShadingLayer layer, bool useMaterial = true);
	virtual IConstantBuffer* GetConstantBuffer(ECBFrequency freq, size_t size /* = 0 */) override;

	DX11ConstantBuffer& GetPerFramePSCB() { return m_PSPerFrameBuff; }
	DX11ConstantBuffer& GetPerFrameVSCB() { return m_VSPerFrameBuff; }

	DX11ConstantBuffer& GetPerInstancePSCB() { return m_PSPerInstanceBuff; }
	DX11ConstantBuffer& GetPerInstanceVSCB() { return m_VSPerInstanceBuff; }
	
	void DrawTexture(DX11Texture* tex, ivec2 pos = ivec2(0), ivec2 size = ivec2(-1));
	void PrepareMesh(MeshPart const& mesh, DX11IndexedMesh& meshData);
	void PrepareMaterial(MaterialID mid);
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
	void SetConstantBuffers(EShaderType shader, IConstantBuffer** buffers, u32 numBuffers) override;
	void SetConstantBuffers(EShaderType shaderType, std::span<CBHandle> handles);
	void ResolveMultisampled(DeviceSubresource const& Dest, DeviceSubresource const& Src);
	void	 UpdateConstantBuffer(CBHandle handle, std::span<const byte> data);
	// End IRenderDeviceCtx overrides



	void Resize(u32 width, u32 height, u32* canvas = nullptr) override;

	void PreImgui();


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

	DX11Material* GetDefaultMaterial(int matType);

	IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData) override;
	IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData) override;

 /* inline static Colour_t GetColour(MaterialID mat)
	{
		return mat.colour;
	}
	inline static int GetSpecularity(MaterialID mat) {
		return mat.specularity;
	}*/

	//constexpr static u32 const MAT_PLAIN = 0;
	//constexpr static u32 const MAT_TEX = 1;
	//constexpr static u32 const MAT_BG = 2;
	//constexpr static u32 const MAT_2D = 3;
	//constexpr static u32 const MAT_POINT_SHADOW_DEPTH = 4;
	//constexpr static u32 const MAT_COUNT = 5;

	std::vector<DX11MaterialType> m_MatTypes;
	std::vector<std::unique_ptr<DX11Material>> m_Materials;
	std::unique_ptr<DX11Cubemap> m_BG;
	DX11Ctx m_Ctx;

	Viewport* GetViewport()
	{
		return mViewport.get();
	}


	virtual void SetShaderResources(EShaderType shader, Vector<ResourceView> const& srvs, u32 startIdx) override;
	void SetPixelShader(PixelShader const* shader) override;
	void SetVertexShader(VertexShader const* shader) override;


	void DrawMesh(Primitive const& primitive) override;
	void SetVertexLayout(VertAttDescHandle attDescHandle) override;
	void UpdateInputLayout();

	VertAttDescHandle mCurrVertexLayoutHdl = -1;
	VertexAttributeDesc const* mCurrVertexLayout = nullptr;
	RefPtr<VertexShader const> mCurrVertexShader = nullptr;


 void DrawMesh(IDeviceMesh* mesh) override;


void Copy(DeviceResourceRef dst, DeviceResourceRef src) override;

 private:
	template<typename TFunc>
	void ForEachMesh(TFunc&& func);
	DX11Texture::Ref PrepareTexture(Texture const& tex, bool sRGB = false);

	RenderContext* mRCtx = nullptr;

protected:

	void CreateMatType(String const& name, u32 index, char const* vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload);
	void CreateMatTypeUnshaded(String const& name, u32 index, char const* vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload, Vector<D3D_SHADER_MACRO>&& defines = {});

	void CreateDepthStencilState(ComPtr<ID3D11DepthStencilState>& state, EDepthMode depth, StencilState stencil);

	UserCamera* m_Camera;
	//Scene* m_Scene;

	ID3D11Device*		   pDevice;
	ID3D11DeviceContext*	   pContext;
	IDXGISwapChain* pSwapChain = nullptr;
	DX11CBPool mCBPool;

//	std::vector<DX11Mesh> m_MeshData;

	DX11Texture::Ref m_EmptyTexture;

	bool mRefactor = true;

	DX11ConstantBuffer m_VSPerInstanceBuff;
	DX11ConstantBuffer m_PSPerInstanceBuff;

	DX11ConstantBuffer m_PSPerFrameBuff;
//	ComPtr<ID3D11Buffer> m_VSPerFrameBuff;
	DX11ConstantBuffer m_VSPerFrameBuff;
	
	ComPtr<ID3D11Buffer> m_ShadowMapCBuff;

	ComPtr<ID3D11VertexShader> m_VertexShader;
	ComPtr<ID3D11PixelShader> m_PlainPixelShader;
	ComPtr<ID3D11PixelShader> m_TexPixelShader;
	ComPtr<ID3D11InputLayout> m_InputLayout;

	ComPtr<ID3D11SamplerState> m_Sampler;
	ComPtr<ID3D11BlendState> m_BlendState;
	ComPtr<ID3D11BlendState> m_AlphaBlendState;
	ComPtr<ID3D11BlendState> m_AlphaLitBlendState;
	EnumArray<EnumArray<ComPtr<ID3D11DepthStencilState>, EStencilMode>, EDepthMode> m_DSStates;

	ComPtr<ID3D11SamplerState> m_ShadowSampler;

	Vector<DX11ShadowMap> m_SpotLightShadows;
	Vector<DX11ShadowMap> m_DirLightShadows;
	Vector<DX11ShadowCube> m_PointLightShadows;

	std::shared_ptr<DX11RenderTarget> m_MainRenderTarget;
	std::shared_ptr<DX11DepthStencil> m_MainDepthStencil;

	Vector<DX11Texture*> m_TextureRegistry;
	Vector<DX11Cubemap*> m_CubemapRegistry;

	OwningPtr<Viewport> mViewport;

	DX11Texture::Ref m_Background;
	ComPtr<ID3D11Buffer> m_BGVBuff;

	ComPtr<ID3D11Buffer> m_2DVBuff;
	DX11ConstantBuffer m_VS2DCBuff;
	DX11ConstantBuffer m_PS2DCBuff;

	DX11Device* myDevice = nullptr;

	std::unordered_map<MeshPart const*, PerInstanceVSData> m_PIVS;

	DX11Texture* m_ViewerTex = nullptr;


	float m_Scale;
	u32 m_Width;
	u32 m_Height;
	std::vector<u32> m_PixelWidth;
	bool m_Dirty = false;

	u32 m_PixelScale = 500;
	float xoffset;
	float yoffset;
	mat4 m_Projection;

	Scene* m_Scene;

	bool m_TextureViewer = false;
	float m_TexViewExp = 1.f;
	float m_DirShadowSize = 10;
	float m_PointShadowFactor = 0.5f;

	StencilState mStencilState;
	EDepthMode mDepthMode = EDepthMode::Less;
	
	bool mUsePasses = true;

};

using DX11DeviceCtx = DX11Renderer;

}
}
