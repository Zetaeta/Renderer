#pragma once
#include "Renderer.h"
#include <vector>
#include "Scene.h"
#include <array>
#include "RastRenderer.h"
#include <d3d11.h>
#include "WinUtils.h"
#include "ImageDX11.h"
#include "DX11Material.h"
#include "CubemapDX11.h"
#include "DX11Ctx.h"

struct DX11Mesh
{
	ComPtr<ID3D11Buffer> vBuff;
	ComPtr<ID3D11Buffer> iBuff;
};

struct PerInstanceVSData
{
	mat4 fullTransform;
	mat4 model2ShadeSpace;
	mat4 model2ShadeDual;
};


class DX11Renderer : public IRenderer
{
public:
	DX11Renderer(Camera* camera, u32 width, u32 height, ID3D11Device* device, ID3D11DeviceContext* context)
		: m_Height(height), m_Width(width), m_Camera(camera), m_PixelWidth(width), pDevice(device), pContext(context)
	{
		Setup();
		m_Ctx.pDevice = pDevice;
		m_Ctx.pContext = pContext;
		memset(m_LayersEnabled, 1, Denum(EShadingLayer::COUNT));
	}

	void DrawControls();

	void Setup();

	void Render(const Scene& scene) override;
	void Render(const Scene& scene, EShadingLayer layer, int index=-1);

	void LoadShaders();

	void PrepareBG();
	void DrawBG();

	template<typename T>
	void CreateConstantBuffer(ComPtr<ID3D11Buffer>& buffer, T const& initialData = T {}, u32 size = sizeof(T));

	template<typename T>
	void WriteCBuffer(ComPtr<ID3D11Buffer>& buffer, T const& data);

	void PrepareShadowMaps();
	void RenderShadowMap();
	void RenderDepthOnly();

	void DrawMesh(MeshRef meshId, EShadingLayer layer);

	void PrepareMesh(Mesh const& mesh, DX11Mesh& meshData);

	void PrepareMaterial(MaterialID mid);

	void SetMainRenderTarget(ComPtr<ID3D11RenderTargetView> rt, ComPtr<ID3D11DepthStencilView> ds)
	{
		m_MainRenderTarget = rt;
		m_MainDepthStencil = ds;
	}

	void Resize(u32 width, u32 height, u32* canvas = nullptr) override;

private:
	ImageDX11::Ref PrepareTexture(Texture const& tex, bool sRGB = false);

/* inline static Colour_t GetColour(MaterialID mat)
	{
		return mat.colour;
	}
	inline static int GetSpecularity(MaterialID mat) {
		return mat.specularity;
	}*/

protected:

	void CreateMatType(u32 index, char const*  vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize);
	void CreateMatTypeUnshaded(u32 index, char const*  vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize);

	Camera* m_Camera;
	//Scene* m_Scene;

	ID3D11Device*		   pDevice;
	ID3D11DeviceContext*	   pContext;

	DX11Ctx m_Ctx;

	std::vector<DX11Mesh> m_MeshData;
	std::vector<std::unique_ptr<DX11Material>> m_Materials;

	ImageDX11::Ref m_EmptyTexture;

	constexpr static u32 const MAT_PLAIN = 0;
	constexpr static u32 const MAT_TEX = 1;
	constexpr static u32 const MAT_BG = 2;
	constexpr static u32 const MAT_COUNT = 3;

	std::vector<DX11MaterialType> m_MatTypes;

	ComPtr<ID3D11Buffer> m_VSPerInstanceBuff;
	ComPtr<ID3D11Buffer> m_PSPerInstanceBuff;

	ComPtr<ID3D11Buffer> m_PSPerFrameBuff;
	ComPtr<ID3D11Buffer> m_VSPerFrameBuff;
	
	ComPtr<ID3D11Buffer> m_ShadowMapCBuff;

	ComPtr<ID3D11VertexShader> m_VertexShader;
	ComPtr<ID3D11PixelShader> m_PlainPixelShader;
	ComPtr<ID3D11PixelShader> m_TexPixelShader;
	ComPtr<ID3D11InputLayout> m_InputLayout;

	ComPtr<ID3D11SamplerState> m_Sampler;
	ComPtr<ID3D11BlendState> m_BlendState;
	ComPtr<ID3D11DepthStencilState> m_LightsDSState;

	//ComPtr<ID3D11Texture2D> m_ShadowMap;
	//ComPtr<ID3D11DepthStencilView> m_ShadowDepthView;
	//ComPtr<ID3D11ShaderResourceView> m_ShadowSRV;
	ComPtr<ID3D11SamplerState> m_ShadowSampler;

	Vector<OwningPtr<DX11ShadowMap>> m_SpotLightShadows;

	ComPtr<ID3D11RenderTargetView> m_MainRenderTarget;
	ComPtr<ID3D11DepthStencilView> m_MainDepthStencil;

	ImageDX11::Ref m_Background;
	CubemapDX11 m_BG;
	ComPtr<ID3D11Buffer> m_BGVBuff;

	Vector<PerInstanceVSData> m_PIVS;

	//Vector<ComPtr<ID3D11

	float m_Scale;
	u32 m_Width;
	u32 m_Height;
	std::vector<u32> m_PixelWidth;
	bool m_Dirty = false;

	u32 m_PixelScale = 500;
	float xoffset;
	float yoffset;
	mat4 m_Projection;

	Scene const* m_Scene;

	bool m_LayersEnabled[Denum(EShadingLayer::COUNT)];
};

