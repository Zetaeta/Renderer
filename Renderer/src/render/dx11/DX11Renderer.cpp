#include "render/dx11/DX11Renderer.h"
#include "scene/Scene.h"
#include <functional>
#include <algorithm>
#include <format>
#include <filesystem>
#include "scene/UserCamera.h"
#include <utility>
#include "render/dx11/DX11Backend.h"
#include <d3d11.h>
#include "core/WinUtils.h"
#include <d3dcompiler.h>
#include "render/dx11/DX11Cubemap.h"
#include "scene/SceneComponent.h"
#include "imgui.h"
#include "glm/gtx/transform.hpp"
#include <core/Matrix.h>
#include <editor/Viewport.h>
#include "render/Shader.h"
#include "render/VertexTypes.h"
#include "../VertexAttributes.h"
#include "render/ForwardRenderPass.h"
#include "render/ShadingCommon.h"
#include "render/dxcommon/DXGIUtils.h"

#pragma comment(lib, "dxguid.lib")

namespace rnd
{
namespace dx11
{
using std::swap;
using std::vector;
using std::array;
using std::begin;
using std::end;
using std::pair;
namespace fs = std::filesystem;

#define ALIGN16(Type, name) \
	Type name;              \
	u8	 _pad_##name[16 - sizeof(Type)];


using DX11Bool = u32;

__declspec(align(16))
struct VS2DCBuff
{
	float2 pos;
	float2 size;
};

__declspec(align(16))
struct PS2DCBuff
{
	float exponent = 1.f;
	int renderMode = 0; // 1=depth, 2=uint
};

template<typename... Args>
struct TMaxSize
{
};
template<typename T, typename... Args>
struct TMaxSize<T, Args...>
{
#ifdef max
#undef max
#endif
	constexpr static u32 size = std::max<u32>(sizeof(T), TMaxSize<Args...>::size);
};

template<>
struct TMaxSize<>
{
	constexpr static u32 size = 0;
};

template<typename... Args>
constexpr u32 MaxSize()
{
	return TMaxSize<Args...>::size;
}

//template<typename T, typename... Args>
//constexpr size_t MaxSize()
//{
//	return std::max(sizeof(T), MaxSize<Args...>());
//}
//
//template<>
//constexpr size_t MaxSize<>()
//{
//	return 0;
//}


__declspec(align(16))
struct PerFrameVertexData
{
	DECLARE_STI_NOBASE(PerFrameVertexData);
	mat4 screen2World;
	mat4 world2Light;
	vec3 cameraPos;
};
DECLARE_CLASS_TYPEINFO(PerFrameVertexData);

//template <typename T>
//void DX11Renderer::CreateConstantBuffer(ComPtr<ID3D11Buffer>& buffer, T const& initialData, u32 size)
//{
//	D3D11_BUFFER_DESC cbDesc;
//	Zero(cbDesc);
//	cbDesc.ByteWidth = size;
//	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
//	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//
//	D3D11_SUBRESOURCE_DATA srData;
//	Zero(srData);
//	srData.pSysMem = Addr(initialData);
//
//	HR_ERR_CHECK(pDevice->CreateBuffer(&cbDesc, &srData,&buffer))
//}


void DX11Renderer::PrepareShadowMaps()
{

	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 1.0f;
	comparisonSamplerDesc.BorderColor[1] = 1.0f;
	comparisonSamplerDesc.BorderColor[2] = 1.0f;
	comparisonSamplerDesc.BorderColor[3] = 1.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;

	// Point filtered shadows can be faster, and may be a good choice when
	// rendering on hardware with lower feature levels. This sample has a
	// UI option to enable/disable filtering so you can see the difference
	// in quality and speed.

	HR_ERR_CHECK(pDevice->CreateSamplerState(
			&comparisonSamplerDesc,
			&m_ShadowSampler));
//	CreateConstantBuffer<ShadowMapCBuff>(m_ShadowMapCBuff);
}

template <typename TFunc>
void DX11Renderer::ForEachMesh(TFunc&& func)
{
	m_Scene->ForEach<StaticMeshComponent>([&](StaticMeshComponent const& smc) {
		auto cmesh = smc.GetMesh();
		auto const&			trans = smc.GetWorldTransform();
		if (!IsValid(cmesh))
		{
			return;
		}

		for (auto const& mesh : cmesh->components)
		{
			func(mesh, trans);
		}
	});
			
}


__declspec(align(16))
struct PerInstancePSData
{
	DECLARE_STI_NOBASE(PerInstancePSData);
	vec4 colour;
	vec3 ambdiffspec;
	float roughness = 1.f;
	vec3 emissiveColour = vec3{0};
	float alphaMask = 1.f;
	float metalness = 1.f;
	DX11Bool useNormalMap;
	DX11Bool useEmissiveMap;
	DX11Bool useRoughnessMap;
};
DECLARE_CLASS_TYPEINFO(PerInstancePSData);

 DX11Renderer::DX11Renderer(Scene* scene, UserCamera* camera, u32 width, u32 height, ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain)
	: DX11Device(device), m_Height(height), m_Width(width), m_Camera(camera), m_PixelWidth(width), pDevice(device), pContext(context), m_Scene(scene), pSwapChain(swapChain), mCBPool(pDevice, this)
{
	mCtx = &m_Ctx;
	CBPool = &mCBPool;
	Device = this;
	myDevice = this;
	TextureManager = &m_Ctx.psTextures;
	m_Ctx.pDevice = pDevice;
	m_Ctx.pContext = pContext;
	m_Ctx.m_Renderer = this;
	Setup();
	MatManager = new MaterialManager(this);
	mViewport = MakeOwning<Viewport>(this, scene, camera);

	D3D11_QUERY_DESC disjointDesc {};
	disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	for (int i = 0; i < DX11Timer::BufferSize; ++i)
	{
		pDevice->CreateQuery(&disjointDesc, &DisjointQueries[i]);
	}
	mFrameTimer = CreateTimer(L"Frame");
	pContext->QueryInterface<ID3DUserDefinedAnnotation>(&mUserAnnotation);
}

DX11Renderer::~DX11Renderer()
{
	mTextures.clear();
	m_Materials.clear();
	m_PointLightShadows.clear();
	m_DirLightShadows.clear();
	m_SpotLightShadows.clear();
	m_Ctx.psTextures = {};
	m_BG = nullptr;
	mRCtx = {};
	m_Background = nullptr;
	m_EmptyTexture = nullptr;
 }

void DX11Renderer::DrawControls()
{
	if (!m_Ctx.mRCtx)
	{
		return;
	}
	ImGui::Begin("Renderer");
	ImGui::Text("GPU time %f", mFrameTimer->GPUTimeMs);
	ImGui::Checkbox("Refactor", &mUsePasses);
	static const char* layers[] = { "base",
		"dirlight",
		"pointlight",
		"spotlight",
	};
	for (int i=0; i<Denum(EShadingLayer::ForwardRenderCount); ++i)
	{
		ImGui::Checkbox(layers[i], &m_Ctx.mRCtx->Settings.LayersEnabled[i]);
	}
	if (ImGui::Button("Reload shaders"))
	{
		LoadShaders();
	}
	if (ImGui::Button("Full recompile shaders"))
	{
		LoadShaders(true);
		mShaderMgr.RecompileAll(true);
	}

	ImGui::DragFloat("Dir shadowmap scale", &m_DirShadowSize);
	ImGui::DragFloat("Point shadow factor", &m_PointShadowFactor, 0.1f);
	ImGui::DragInt("Debug mode", reinterpret_cast<int*>(&mRCtx->Settings.ShadingDebugMode), 0.5, 0, Denum(EShadingDebugMode::COUNT));
	ImGui::DragFloat("Debug greyscale exponent", &mRCtx->Settings.DebugGrayscaleExp, 0.1f, 0, 10);
	ImGui::DragInt2("Viewer size", &mViewerSize.x, 1.f, 0, 2500);
	mRCtx->DrawControls();

	{
		if (ImGui::BeginCombo("Texture viewer", m_ViewerTex == nullptr ? "None" : std::format("{}##{}", m_ViewerTex->IsDepthStencil() ? "Depth" : "Texture", reinterpret_cast<u64> (m_ViewerTex)).c_str()))
		{
			{
				bool selected = m_ViewerTex == nullptr;
				if (ImGui::Selectable("None##1", selected))
				{
					m_ViewerTex = nullptr;
				}
				if (selected)
                    ImGui::SetItemDefaultFocus();
			}
			for (auto tex : m_TextureRegistry)
			{
				String name = std::format("{}##{}", tex->Desc.DebugName.empty() ? (tex->IsDepthStencil() ? "Depth" : "Texture") : tex->Desc.DebugName.c_str(), reinterpret_cast<u64>(tex));
				bool selected = m_ViewerTex == tex;
				if (ImGui::Selectable(name.c_str(), selected))
				{
					m_ViewerTex = tex;
				}
				if (selected)
                    ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		IDeviceTextureCube*& cube = mRCtx->mDebugCube;
		if (ImGui::BeginCombo("Cubemap viewer", cube == nullptr ? "None" : std::format("{}##{}", cube->Desc.DebugName.empty() ? (cube->IsDepthStencil() ? "Depth" : "Texture") : cube->Desc.DebugName.c_str(), reinterpret_cast<u64> (cube)).c_str()))
		{
			{
				bool selected = cube == nullptr;
				if (ImGui::Selectable("None##1", selected))
				{
					mRCtx->SetDebugCube(nullptr);
				}
				if (selected)
                    ImGui::SetItemDefaultFocus();
			}
			for (auto tex : m_CubemapRegistry)
			{
				String name = std::format("{}##{}", tex->Desc.DebugName.empty() ? (tex->IsDepthStencil() ? "Depth" : "Texture") : tex->Desc.DebugName.c_str(), reinterpret_cast<u64>(tex));
				bool selected = cube == tex;
				if (ImGui::Selectable(name.c_str(), selected))
				{
					mRCtx->SetDebugCube(tex);
				}
				if (selected)
                    ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::DragFloat("Exponent", &m_TexViewExp, 0.01f, 0.01f, 10.f);

		//if (m_ViewerTex != nullptr)
		//{
		//	ImGui::Image(m_ViewerTex->GetSRV(), {500, 500}, {0,1}, {1,0});
		//}
	}

	//ImGui::Selectable()

	ImGui::End();
}

void DX11Renderer::Setup()
{


	//Setup matrix constant buffer
	m_VSPerInstanceBuff = DX11ConstantBuffer::Create<PerInstanceVSData>(&m_Ctx);
	m_VSPerFrameBuff = DX11ConstantBuffer::CreateWithLayout<PerFrameVertexData>(&m_Ctx);

	m_PSPerInstanceBuff = DX11ConstantBuffer::Create<PerInstancePSData>(&m_Ctx);

	m_PSPerFrameBuff = DX11ConstantBuffer(&m_Ctx, MaxSize<PFPSPointLight, PFPSSpotLight, PerFramePSData>());


	m_MatTypes.resize(MAT_COUNT);
	LoadShaders();
	//CreateMatType(MAT_PLAIN, "PlainVertexShader","PlainPixelShader",ied,Size(ied));
	//CreateMatType(MAT_TEX, "TexVertexShader","TexPixelShader",ied,Size(ied));

	u32 const empty = 0x00000000;
	DeviceTextureDesc emptyDesc;
	emptyDesc.Width = 1;
	emptyDesc.Height = 1;
	emptyDesc.DebugName = "Empty";
	m_EmptyTexture = std::make_shared<DX11Texture>(m_Ctx, emptyDesc, &empty);

	{
		D3D11_SAMPLER_DESC sDesc;
		Zero(sDesc);
		sDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sDesc.MaxLOD = D3D11_FLOAT32_MAX;
		HR_ERR_CHECK(pDevice->CreateSamplerState(&sDesc, &m_Sampler));
	}
	PrepareShadowMaps();

	ID3D11SamplerState* samplers[] = { m_Sampler.Get(), m_ShadowSampler.Get() };

	pContext->PSSetSamplers(0, 2, samplers );

	PrepareBG();

	{
		D3D11_BLEND_DESC blendDesc;
		Zero(blendDesc);
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HR_ERR_CHECK(pDevice->CreateBlendState(&blendDesc, &m_BlendState));
	}
	{
		D3D11_BLEND_DESC blendDesc;
		Zero(blendDesc);
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HR_ERR_CHECK(pDevice->CreateBlendState(&blendDesc, &m_AlphaBlendState));
	}
	{
		D3D11_BLEND_DESC blendDesc;
		Zero(blendDesc);
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HR_ERR_CHECK(pDevice->CreateBlendState(&blendDesc, &m_AlphaLitBlendState));
	}

	// 2D
	{
		

		float const Z = -0.f;
		FlatVert const verts[] = {
			{ {-1,-1.f, Z}, {0,1} },
			{ {-1,1.f, Z}, {0,0} },
			{ {1,-1.f, Z}, {1,1} },
			{ {1,-1.f, Z}, {1,1} },
			{ {-1,1.f, Z}, {0,0} },
			{ {1,1.f, Z}, {1,0} },
		};
		const UINT stride = Sizeof(verts[0]);
		{
			D3D11_BUFFER_DESC bd = {};
			Zero(bd);
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = Sizeof(verts);
			bd.StructureByteStride = stride;
			D3D11_SUBRESOURCE_DATA sd;
			Zero(sd);
			sd.pSysMem = Addr(verts);

			HR_ERR_CHECK(pDevice->CreateBuffer(&bd, &sd, &m_2DVBuff));
		}
		m_VS2DCBuff = DX11ConstantBuffer::Create<VS2DCBuff>(&m_Ctx);
		m_PS2DCBuff = DX11ConstantBuffer::Create<PS2DCBuff>(&m_Ctx);
	}

}
void DX11Renderer::PrepareBG()
{
	float const Z = 0.9999f;
	FlatVert const verts[3] = {
		{ {-1,-1.f, Z}, {0,2} },
		{ {-1,3.f, Z}, {0,0} },
		{ {3,-1.f, Z}, {2,2} },
	};
	const UINT stride = Sizeof(verts[0]);
	{
		D3D11_BUFFER_DESC bd = {};
		Zero(bd);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = Sizeof(verts);
		bd.StructureByteStride = stride;
		D3D11_SUBRESOURCE_DATA sd;
		Zero(sd);
		sd.pSysMem = Addr(verts);

		HR_ERR_CHECK(pDevice->CreateBuffer(&bd, &sd, &m_BGVBuff));
	}
	TextureRef tex = Texture::LoadFrom("content/canyon2.jpg");
	m_Background = DX11Texture::CreateFrom(tex, &m_Ctx);
	m_BG = std::make_unique<DX11Cubemap>(DX11Cubemap::FoldUp(m_Ctx, tex));
}

void DX11Renderer::Render(const Scene& scene)
{
	++mFrameNum;
	ProcessTimers();
	StartFrameTimer();
	ProcessTextureCreations();
	ID3D11SamplerState* samplers[] = { m_Sampler.Get(), m_ShadowSampler.Get() };

	pContext->PSSetSamplers(0, 2, samplers );
	m_Scene = const_cast<Scene*>(&scene);
	m_Materials.resize(scene.Materials().size());

	ImVec4 clear_color = ImVec4();
	auto   RTV = m_MainRenderTarget->GetRTV();
	pContext->OMSetRenderTargets(1, &RTV, NULL_OR(m_MainDepthStencil, GetDSV()));

	const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
	//pContext->ClearRenderTargetView(m_MainRenderTarget->GetRTV(), clear_color_with_alpha);
	//pContext->ClearDepthStencilView(m_MainDepthStencil->GetDSV(), D3D11_CLEAR_DEPTH, 1.f, 0);

	m_Scale = float(std::min(m_Width, m_Height));
	m_Camera->SetViewExtent(.5f * m_Width / m_Scale, .5f * m_Height / m_Scale );

	D3D11_VIEWPORT vp = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = float(m_Width),
		.Height = float(m_Height),
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	pContext->RSSetViewports(1u, &vp);

	if (mUsePasses)
	{
		m_Ctx.mRCtx->RenderFrame(scene);
	}
	else
	{

		//mat4
		mat4 worldToCamera = m_Camera->WorldToCamera();
		float const FAR_PLANE = 1000;
		float const NEAR_PLANE = .1f;
		m_Projection =
		{ 
			{ 2 * m_Scale / float(m_Width), 0, 0, 0 },
			{0,			2 * m_Scale / float(m_Height),0,0},
			{ 0, 0, (FAR_PLANE) / (FAR_PLANE - NEAR_PLANE), -FAR_PLANE * NEAR_PLANE  / (FAR_PLANE - NEAR_PLANE)},
			{0,			0,		1,0},
		};
		m_Projection = glm::transpose(m_Projection);
		mat4 projWorld = m_Projection * worldToCamera;

		PerFrameVertexData PFVD;
		PFVD.cameraPos = m_Camera->GetPosition();
		PFVD.screen2World = inverse(projWorld);
		m_VSPerFrameBuff.WriteData(PFVD);

		//m_PIVS.resize(scene.m_MeshInstances.size());
		ForEachMesh([&](MeshPart const& mesh, Transform const& trans) {
			auto& mat = m_Scene->GetMaterial(mesh.material);

			PerInstanceVSData& PIVS = m_PIVS[&mesh];
			PIVS.model2ShadeSpace = mat4(trans);
			PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
			PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

			m_VSPerInstanceBuff.WriteData(PIVS);
		});
		for (u32 i=0; i< scene.m_MeshInstances.size(); ++i)
		{

			//ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
			//pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
			//DrawMesh(mi.mesh);
		}
		pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::BASE)])
		{
			Render(scene, EShadingLayer::BASE, -1, false);
		}
		pContext->OMSetBlendState(m_BlendState.Get(), nullptr, 0xffffffff);
		//pContext->OMSetDepthStencilState(m_LightsDSState.Get(), 1);
		SetDepthStencilMode(EDepthMode::Equal | EDepthMode::NoWrite);

		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::DIRLIGHT)])
		{
			for (int i = 0; i < scene.m_DirLights.size(); ++i)
			{
				Render(scene, EShadingLayer::DIRLIGHT, i, false);
			}
		}
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::SPOTLIGHT)])
		{
			for (int i = 0; i < scene.m_SpotLights.size(); ++i)
			{
				Render(scene, EShadingLayer::SPOTLIGHT, i,false);
			}
		}
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::POINTLIGHT)])
		{
			for (int i = 0; i < scene.m_PointLights.size(); ++i)
			{
				Render(scene, EShadingLayer::POINTLIGHT, i,false);
			}
		}

		pContext->OMSetBlendState(m_AlphaBlendState.Get(), nullptr, 0xffffffff);
	//	pContext->OMSetDepthStencilState(m_AlphaDSState.Get(), 1);
		SetDepthStencilMode(EDepthMode::LessEqual | EDepthMode::NoWrite);
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::BASE)])
		{
			Render(scene, EShadingLayer::BASE, -1, true);
		}

		pContext->OMSetBlendState(m_AlphaLitBlendState.Get(), nullptr, 0xffffffff);
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::DIRLIGHT)])
		{
			for (int i = 0; i < scene.m_DirLights.size(); ++i)
			{
				Render(scene, EShadingLayer::DIRLIGHT, i, true);
			}
		}
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::SPOTLIGHT)])
		{
			for (int i = 0; i < scene.m_SpotLights.size(); ++i)
			{
				Render(scene, EShadingLayer::SPOTLIGHT, i, true);
			}
		}
		if (mRCtx->Settings.LayersEnabled[Denum(EShadingLayer::POINTLIGHT)])
		{
			for (int i = 0; i < scene.m_PointLights.size(); ++i)
			{
				Render(scene, EShadingLayer::POINTLIGHT, i, true);
			}
		}
		pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	//	pContext->OMSetDepthStencilState(nullptr, 1);
		SetDepthStencilMode(EDepthMode::Less);
		DrawBG();
	}



	SetDepthStencilMode(EDepthMode::Disabled);
	if (m_ViewerTex != nullptr)
	{
		DrawTexture(m_ViewerTex);
	}
	pContext->OMSetRenderTargets(0, nullptr, nullptr);

	pContext->PSSetShader(nullptr, nullptr, 0);
	pContext->VSSetShader(nullptr, nullptr, 0);

	m_Ctx.psTextures.UnBind(m_Ctx);
	EndFrameTimer();
}


void DX11Renderer::Render(const Scene& scene, EShadingLayer layer, int index, bool translucent)
{
	switch (layer)
	{
	case EShadingLayer::BASE:
	{
		PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(scene.m_AmbientLight);
		m_PSPerFrameBuff.WriteData(perFrameData);
		break;
	}
	case EShadingLayer::DIRLIGHT:
	{
		auto const& light = scene.m_PointLights[index];
		PFPSDirLight perFrameData;
		Zero(perFrameData);
		perFrameData.directionalCol = vec3(scene.m_DirLights[index].colour);
		perFrameData.directionalDir = scene.m_DirLights[index].dir;
		perFrameData.world2Light = m_DirLightShadows[index].m_World2LightProj;
		m_PSPerFrameBuff.WriteData(perFrameData);
		m_Ctx.psTextures.SetTexture(E_TS_SHADOWMAP, m_DirLightShadows[index].GetSRV());
		break;
	}
	case EShadingLayer::SPOTLIGHT:
	{
		auto const& light = scene.m_SpotLights[index];
		PFPSSpotLight perFrameData;
		Zero(perFrameData);
		perFrameData.spotLightCol = light.colour;
		perFrameData.spotLightDir = light.dir;
		perFrameData.spotLightPos = light.pos;
		perFrameData.spotLightTan = light.tangle;
		perFrameData.spotLightFalloff = light.falloff;
		perFrameData.world2Light = m_SpotLightShadows[index].m_World2LightProj;

		m_PSPerFrameBuff.WriteData(perFrameData);

		m_Ctx.psTextures.SetTexture(E_TS_SHADOWMAP, m_SpotLightShadows[index].GetSRV());
		break;
	}
	case EShadingLayer::POINTLIGHT:
	{
		auto const& light = scene.m_PointLights[index];
		PFPSPointLight perFrameData;
		Zero(perFrameData);
		perFrameData.pointLightPos = light.pos;
		perFrameData.pointLightCol = light.colour;
		perFrameData.pointLightRad = light.radius;
		perFrameData.pointLightFalloff = light.falloff;
		perFrameData.world2Light = m_PointLightShadows[index].m_World2Light;
		m_PSPerFrameBuff.WriteData(perFrameData);

		m_Ctx.psTextures.SetTexture(E_TS_SHADOWMAP, m_PointLightShadows[index].GetSRV());
		break;
	}
	default:
		break;
	}
	ForEachMesh([&] (MeshPart const& mesh, Transform const& trans)
	{
		if (translucent != m_Scene->GetMaterial(mesh.material).translucent)
		{
			return;
		}
		m_VSPerInstanceBuff.WriteData(m_PIVS[&mesh]);

		DrawMesh(mesh, layer);
	});
}

void DX11Renderer::DrawBG()
{
	DrawCubemap(m_BG->GetSRV());
}

void DX11Renderer::DrawCubemap(ID3D11ShaderResourceView* srv, bool depth)
{
	const UINT stride = sizeof(FlatVert);
	const UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, m_BGVBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11Buffer* vbuffs[] = { m_VSPerFrameBuff.Get() };

	if (depth)
	{
		m_MatTypes[MAT_CUBE_DEPTH].Bind(m_Ctx, EShadingLayer::BASE);
	}
	else
	{
		m_MatTypes[MAT_BG].Bind(m_Ctx, EShadingLayer::BASE);
	}
	pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
	//WriteCBuffer(m_VSPerFrameBuff)

	pContext->PSSetShaderResources(0, 1, &srv);
	if (false && m_PointLightShadows.size() > 0 && m_PointLightShadows[0])
	{
		pContext->PSSetShaderResources(0, 1, &srv);
	}

	pContext->Draw(3,0);
}

void DX11Renderer::DrawCubemap(IDeviceTextureCube* cubemap)
{
	DrawCubemap(cubemap->GetTextureHandle<ID3D11ShaderResourceView*>(), cubemap->IsDepthStencil());
}

void DX11Renderer::DrawTexture(DX11Texture* tex, ivec2 pos, ivec2 size )
{
	VS2DCBuff cbuff;
	vec2 texturePos = vec2(pos) / vec2 {m_Width, m_Height};
	cbuff.pos = texturePos;// vec2(texturePos.x * 2 - 1, 1 - 2 * texturePos.y);
	if (size.x <= 0)
		size.x = mViewerSize.x;
	if (size.y <= 0)
		size.y = mViewerSize.y;
	cbuff.size = vec2(size) / vec2 {m_Width, m_Height};

	m_VS2DCBuff.WriteData(cbuff);

	PS2DCBuff pbuff;
	if (tex->IsDepthStencil())
	{
		pbuff.renderMode = 1;
	}
	else if (tex->Desc.Format == ETextureFormat::R32_Uint)
	{
		pbuff.renderMode = 2;
	}

	pbuff.exponent = m_TexViewExp;
	m_PS2DCBuff.WriteData(pbuff);

	const UINT stride = sizeof(FlatVert);
	const UINT offset = 0;

	SetBlendMode(EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE);
	pContext->VSSetConstantBuffers(0, 1, m_VS2DCBuff.GetAddressOf());
	pContext->PSSetConstantBuffers(0, 1, m_PS2DCBuff.GetAddressOf());
	auto* rtv = m_MainRenderTarget->GetRTV();
	pContext->OMSetRenderTargets(1, &rtv, nullptr);

	pContext->IASetVertexBuffers(0, 1, m_2DVBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	if (pbuff.renderMode <= 1)
	{
		m_MatTypes[MAT_2D].Bind(m_Ctx, EShadingLayer::BASE);
	}
	else
	{
		m_PS2DCBuff.WriteData(ivec2(m_Width, m_Height));
		m_MatTypes[MAT_2D_UINT].Bind(m_Ctx, EShadingLayer::BASE);
	}

	auto* srv = tex->GetSRV();
	if (!srv)
	{
		tex->CreateSRV();
		srv = tex->GetSRV();
	}
	pContext->PSSetShaderResources(0,1, &srv);
	pContext->Draw(6, 0);
}


inline int Round(int x) {
	return x;
}

inline int Round(float x) {
	return int(round(x));
}


void DX11Renderer::DrawMesh(MeshPart const& mesh, EShadingLayer layer, bool useMaterial /*= true*/)
{
//	ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
//	pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
	auto& meshData = m_MeshData[&mesh];
	if (!meshData.vBuff)
	{
		PrepareMesh(mesh, meshData);
	}
	assert(meshData.iBuff);

	bool needsUpdate = false;
	auto const& sceneMats = m_Scene->Materials();
	if (RCHECK(mesh.material < sceneMats.size() && sceneMats[mesh.material]))
	{
		needsUpdate = sceneMats[mesh.material]->NeedsUpdate();
	}
	if (!m_Materials[mesh.material] || needsUpdate)
	{
		PrepareMaterial(mesh.material);
	}

	if (useMaterial)
	{
		m_Materials[mesh.material]->Bind(m_Ctx, layer);
	}

	const UINT stride = Sizeof(mesh.vertices[0]);
	const UINT offset = 0;


	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->IASetVertexBuffers(0, 1, meshData.vBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(meshData.iBuff.Get(),DXGI_FORMAT_R16_UINT, 0);
	pContext->DrawIndexed(Size(mesh.triangles) * 3,0,0);
	m_Materials[mesh.material]->UnBind(m_Ctx);
}

void DX11Renderer::DrawMesh(IDeviceMesh* mesh)
{
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11SamplerState* samplers[] = { m_Sampler.Get(), m_ShadowSampler.Get() };
	pContext->PSSetSamplers(0, 2, samplers );
	if (mesh->MeshType == EDeviceMeshType::DIRECT)
	{
		DX11DirectMesh* mesh11 = static_cast<DX11DirectMesh*>(mesh);
		ZE_REQUIRE(mCurrVertexLayout);
		const UINT		stride = mCurrVertexLayout->GetSize();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, mesh11->vBuff.GetAddressOf(), &stride, &offset);
		pContext->Draw(mesh11->VertexCount, 0);
	}
}

void DX11Renderer::Copy(DeviceResourceRef dst, DeviceResourceRef src)
{
	// Invalid cast, but ResourceType will always be in the same place
	switch (static_cast<IDeviceTexture*>(src.get())->Desc.ResourceType)
	{
	case EResourceType::Texture2D:
	{
		DX11Texture* src11 = static_cast<DX11Texture*>(src.get());
		DX11Texture* dst11 = static_cast<DX11Texture*>(dst.get());
		RCHECK(dst11->Desc.ResourceType == EResourceType::Texture2D);
		pContext->CopyResource(dst11->GetTexture(), src11->GetTexture());
		break;
	}
	default:
		ZE_ENSURE(false);
		break;
	}
}

IConstantBuffer* DX11Renderer::GetConstantBuffer(ECBFrequency freq, size_t size /* = 0 */)
{
	DX11ConstantBuffer* result = nullptr;
	switch (freq)
	{
	case rnd::ECBFrequency::PS_PerInstance:
		result = &m_PSPerInstanceBuff;
		break;
	case rnd::ECBFrequency::PS_PerFrame:
		result = &m_PSPerFrameBuff;
		break;
	case rnd::ECBFrequency::VS_PerInstance:
		result = &m_VSPerInstanceBuff;
		break;
	case rnd::ECBFrequency::VS_PerFrame:
		result = &m_VSPerFrameBuff;
		break;
	default:
		return nullptr;
	}
	if (size > result->GetCBData().GetSize())
	{
		result->Resize(size);
	}

	return result;
}

void DX11Renderer::PrepareMesh(MeshPart const& mesh, DX11IndexedMesh& meshData)
{
//	printf("Creating buffers for mesh %s\n", mesh.name.ToString().c_str());
	const UINT stride = Sizeof(mesh.vertices[0]);
	{
		D3D11_BUFFER_DESC bd = {};
		Zero(bd);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = Sizeof(mesh.vertices);
		bd.StructureByteStride = stride;
		D3D11_SUBRESOURCE_DATA sd;
		Zero(sd);
		sd.pSysMem = Addr(mesh.vertices);

		HR_ERR_CHECK(pDevice->CreateBuffer(&bd, &sd, &meshData.vBuff));
	}
	const UINT offset = 0;

	ComPtr<ID3D11Buffer> iBuffer;

	{
		D3D11_BUFFER_DESC bd = {};
		Zero(bd);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = Sizeof(mesh.triangles);
		bd.StructureByteStride = Sizeof(mesh.triangles[0]);
		D3D11_SUBRESOURCE_DATA sd;
		Zero(sd);
		sd.pSysMem = Addr(mesh.triangles);

		HR_ERR_CHECK(pDevice->CreateBuffer(&bd, &sd, &meshData.iBuff));
	}

	meshData.IndexCount = mesh.GetTriCount() * 3;
	meshData.VertexCount = mesh.GetVertCount();
}

void DX11Renderer::PrepareMaterial(MaterialID mid)
{
	Material&					  mat = m_Scene->GetMaterial(mid);
	std::lock_guard lock{ mat.GetUpdateMutex() };
	std::unique_ptr<DX11Material>& result = m_Materials[mid];
	if (mat.albedo->IsValid())
	{
		auto texMat = std::make_unique<DX11TexturedMaterial>(&m_MatTypes[MAT_TEX]);
		if (mat.albedo.IsValid())
		{
			texMat->m_Albedo = PrepareTexture(*mat.albedo, true);
		}
		if (mat.normal.IsValid())
		{
			texMat->m_Normal = PrepareTexture(*mat.normal);
//			mat.normal.Clear();
		}
		if (mat.emissiveMap.IsValid())
		{
			texMat->m_Emissive = PrepareTexture(*mat.emissiveMap);
//			mat.emissive.Clear();
		}
		else
		{
			texMat->m_Emissive = m_EmptyTexture;
		}
		if (mat.roughnessMap.IsValid())
		{
			texMat->m_Roughness = PrepareTexture(*mat.roughnessMap);
		}
		else
		{
			texMat->m_Roughness = m_EmptyTexture;
		}
		result = std::move(texMat);
	}
	else
	{
		result = std::make_unique<DX11Material>(&m_MatTypes[MAT_PLAIN]);
	}
	mat.DeviceMat = result.get();
}

void DX11Renderer::SetBackbuffer(DX11Texture::Ref backBufferTex, u32 width, u32 height)
{
	m_MainRenderTarget = static_pointer_cast<DX11RenderTarget>(backBufferTex->GetRT());

	mViewport->Resize(width, height, backBufferTex);
	m_Ctx.mRCtx = mRCtx = mViewport->GetRenderContext();
	m_MainRenderTarget->Desc.Width = width;
	m_MainRenderTarget->Desc.Height = height;
	m_Width = width;
	m_Height = height;
	m_Camera->SetViewExtent(.5f * m_Width / m_Scale, .5f * m_Height / m_Scale );
}

void DX11Renderer::Resize(u32 width, u32 height, u32* canvas)
{
	m_MainRenderTarget = nullptr;
	mViewport->Reset();
	pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

	if (width > 0 && height > 0)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		DeviceTextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.Flags = TF_RenderTarget | TF_SRV;// | TF_SRGB;
		SetBackbuffer(std::make_shared<rnd::dx11::DX11Texture>(m_Ctx,desc, backBuffer.Get()), width, height);
	}
}

void DX11Renderer::PreImgui()
{
	auto rt = m_MainRenderTarget->GetRTV();
	pContext->OMSetRenderTargets(1, &rt, nullptr);
}

MappedResource DX11Renderer::MapResource(ID3D11Resource* resource, u32 subResource, ECpuAccessFlags flags)
{
	D3D11_MAP mapFlags;
	if (flags == ECpuAccessFlags::Write)
	{
		mapFlags = D3D11_MAP_WRITE_DISCARD;
	}
	else
	{
		ZE_ASSERT(flags != ECpuAccessFlags::None);
		mapFlags = ((flags & ECpuAccessFlags::Write) != 0) ? D3D11_MAP_READ_WRITE : D3D11_MAP_READ;
	}
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HR_ERR_CHECK(m_Ctx.pContext->Map(resource, subResource,  mapFlags, 0, &mapped));

	MappedResource result;
	result.Data = mapped.pData;
	result.RowPitch = mapped.RowPitch;
	result.DepthPitch = mapped.DepthPitch;
	return result;
}

DX11Texture::Ref DX11Renderer::PrepareTexture(Texture const& tex, bool sRGB /*= false*/)
{
	//if (auto* )
	//{
	//	return std::static_pointer_cast<DX11Texture>(tex.GetDeviceTexture());
	//}
	auto result = GetRenderTexture(&tex);
	if (!result)
	{
		return m_EmptyTexture;
	}
	return result;
//	DeviceTextureDesc desc;
//	desc.Flags = (sRGB ? TF_SRGB : TF_NONE) | TF_SRV;
//	desc.Width = tex.width;
//	desc.Height = tex.height;
//	desc.NumMips = 0;
//	desc.DebugName = tex.GetName();
//	desc.Format = ETextureFormat::RGBA8_Unorm;
//	auto			result = std::make_shared<DX11Texture>(m_Ctx, desc, tex.GetData());
////	auto result = DX11Texture::Create(&m_Ctx, tex.width, tex.height, , );
//	tex.SetDeviceTexture(result);
//	return result;
}

void DX11Renderer::ProcessTimers()
{
	u32 frameIdx = mFrameNum % DX11Timer::BufferSize;
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
	if ((pContext->GetData(DisjointQueries[frameIdx].Get(), &disjointData, sizeof(disjointData), 0) != S_OK)
		|| disjointData.Disjoint)
	{
		for (u32 i = 0; i < mTimerPool.Size(); ++i)
		{
			mTimerPool[i].GPUTimeMs = -1.f;
		}
		return;
	}
	for (u32 i = 0; i < mTimerPool.Size(); ++i)
	{
		if (!mTimerPool.IsInUse(i))
		{
			continue;
		}
		u64 start, end;
		DX11Timer& timer = mTimerPool[i];
		if (timer.GetRefCount() == 0)
		{
			mTimerPool.Release(i);
			continue;
		}

		if ((pContext->GetData(timer.Queries[frameIdx].Start.Get(), &start, sizeof(u64), 0) == S_OK)
			&& (pContext->GetData(timer.Queries[frameIdx].End.Get(), &end, sizeof(u64), 0) == S_OK))
		{
			timer.GPUTimeMs = NumCast<float>((double(end - start) * 1000.) / disjointData.Frequency);
		}
		else
		{
			timer.GPUTimeMs = 0;
		}
	}
}

void DX11Renderer::StartFrameTimer()
{
	pContext->Begin(DisjointQueries[mFrameNum % DX11Timer::BufferSize].Get());
	StartTimer(mFrameTimer);
}

void DX11Renderer::EndFrameTimer()
{
	StopTimer(mFrameTimer);
	pContext->End(DisjointQueries[mFrameNum % DX11Timer::BufferSize].Get());
}

void DX11Renderer::UnregisterTexture(DX11Texture* tex)
{
	std::erase_if(m_TextureRegistry, [tex](DX11Texture* other) {return other == tex; });
	if (m_ViewerTex == tex)
	{
		m_ViewerTex = nullptr;
	}
}

void DX11Renderer::UnregisterTexture(DX11Cubemap* tex)
{
	std::erase_if(m_CubemapRegistry, [tex](auto* other) {return other == tex; });
}

DX11Material* DX11Renderer::GetDefaultMaterial(int matType)
{
	auto& materialType = m_MatTypes[matType];
	if (!IsValid(materialType.m_Default))
	{
		materialType.m_Default = std::make_unique<DX11Material>(&materialType);
		
	}
	return materialType.m_Default.get();
}

IDeviceTexture::Ref DX11Renderer::CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData)
{
	return std::make_shared<DX11Cubemap>(m_Ctx, desc);
}

IDeviceTexture::Ref DX11Renderer::CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData)
{
//	return DX11Cubemap::FoldUp(m_Ctx, )
//	return DX11Texture::Create(&m_Ctx, desc.width, desc.height, reinterpret_cast<u32 const*>(initialData), TF_DEPTH);
	return std::make_shared<DX11Texture>(m_Ctx, desc, initialData);
}

void DX11Renderer::SetShaderResources(EShaderType shader, Span<ResourceView const> srvs, u32 startIdx)
{
	Vector<ID3D11ShaderResourceView*> views(srvs.size());
	for (int i=0;i<srvs.size(); ++i)
	{
		views[i] = srvs[i].Get<ID3D11ShaderResourceView*>();
	}

	mMaxShaderResources[shader] = max(mMaxShaderResources[shader], NumCast<u32>(srvs.size() + startIdx));
	
	switch (shader)
	{
		case rnd::EShaderType::Vertex:
			pContext->VSSetShaderResources(startIdx, NumCast<u32>(views.size()), Addr(views));
			break;
		case rnd::EShaderType::Pixel:
			pContext->PSSetShaderResources(startIdx, NumCast<u32>(views.size()), Addr(views));
			break;
		case rnd::EShaderType::Geometry:
			pContext->GSSetShaderResources(startIdx, NumCast<u32>(views.size()), Addr(views));
			break;
		case rnd::EShaderType::Compute:
			pContext->CSSetShaderResources(startIdx, NumCast<u32>(views.size()), Addr(views));
			break;
		default:
			break;
	}
}

void DX11Renderer::SetUAVs(EShaderType shader, Span<UnorderedAccessView const> uavs, u32 startIdx /* = 0 */)
{
	Vector<ID3D11UnorderedAccessView*> views(uavs.size());
	for (int i=0;i<uavs.size(); ++i)
	{
		views[i] = uavs[i].Get<ID3D11UnorderedAccessView*>();
	}
	mMaxUAVs[shader] = max(mMaxUAVs[shader], NumCast<u32>(startIdx + uavs.size()));
	switch (shader)
	{
	case rnd::EShaderType::Pixel:
		pContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, startIdx, NumCast<u32>(views.size()), &views[0], nullptr);
		break;
	case rnd::EShaderType::Compute:
		pContext->CSSetUnorderedAccessViews(startIdx, NumCast<u32>(uavs.size()), &views[0], nullptr);
		break;
	default:
		ZE_ASSERT(false);
		break;
	}
}


void DX11Renderer::UnbindUAVs(EShaderType shader, u32 clearNum, u32 startIdx /* = 0 */)
{

	pContext->CSSetUnorderedAccessViews(startIdx, clearNum, reinterpret_cast<ID3D11UnorderedAccessView* const*>(ZerosArray), nullptr);
}

void DX11Renderer::SetSamplers(EShaderType shader, Span<SamplerHandle const> samplers, u32 startSlot)
{
	static_assert(sizeof(SamplerHandle) == sizeof(ID3D11SamplerState*));
	ID3D11SamplerState* const* const d3d11Samplers = reinterpret_cast<ID3D11SamplerState* const*>(samplers.data());
	u32 numSamplers = NumCast<u32>(samplers.size());
	switch (shader)
	{
	case rnd::EShaderType::Vertex:
		pContext->VSSetSamplers(startSlot, numSamplers, d3d11Samplers);
		break;
	case rnd::EShaderType::Pixel:
		pContext->PSSetSamplers(startSlot, numSamplers, d3d11Samplers);
		break;
	case rnd::EShaderType::Geometry:
		pContext->GSSetSamplers(startSlot, numSamplers, d3d11Samplers);
		break;
	case rnd::EShaderType::Compute:
		pContext->CSSetSamplers(startSlot, numSamplers, d3d11Samplers);
		break;
	default:
		ZE_ASSERT(false);
		break;
	}
}

void DX11Renderer::DispatchCompute(ComputeDispatch args)
{
	pContext->Dispatch(args.ThreadGroupsX, args.ThreadGroupsY, args.ThreadGroupsZ);
}

void DX11Renderer::ClearResourceBindings()
{
	m_Ctx.psTextures.UnBind(m_Ctx);
	ID3D11ShaderResourceView* const* dummyViews = reinterpret_cast<ID3D11ShaderResourceView* const*>(ZerosArray);
	pContext->PSSetShaderResources(0, mMaxShaderResources[EShaderType::Pixel], dummyViews);
	pContext->VSSetShaderResources(0, mMaxShaderResources[EShaderType::Vertex], dummyViews);
	pContext->CSSetShaderResources(0, mMaxShaderResources[EShaderType::Compute], dummyViews);
	pContext->CSSetUnorderedAccessViews(0, mMaxUAVs[EShaderType::Compute], reinterpret_cast<ID3D11UnorderedAccessView* const*>(ZerosArray), nullptr);
	pContext->OMSetRenderTargets(0, nullptr, nullptr);
}

#if PROFILING
GPUTimer* DX11Renderer::CreateTimer(const wchar_t* name)
{
	DX11Timer* timer;
	u32 dontCare;
	bool isNew = mTimerPool.Claim(dontCare, timer);
	timer->Name = name;

	if (isNew)
	{
		D3D11_QUERY_DESC desc {};
		desc.Query = D3D11_QUERY_TIMESTAMP;
		for (int i = 0; i < DX11Timer::BufferSize; ++i)
		{
			pDevice->CreateQuery(&desc, &timer->Queries[i].Start);
			pDevice->CreateQuery(&desc, &timer->Queries[i].End);
		}
	}
	return timer;
}

void DX11Renderer::StartTimer(GPUTimer* timer)
{
	u32 currFrameIdx = NumCast<u32>(mFrameNum % DX11Timer::BufferSize);
	pContext->End(static_cast<DX11Timer*>(timer)->Queries[currFrameIdx].Start.Get());
	if (mUserAnnotation)
	{
		mUserAnnotation->BeginEvent(timer->Name.c_str());
	}
}

void DX11Renderer::StopTimer(GPUTimer* timer)
{

	u32 currFrameIdx = NumCast<u32>(mFrameNum % DX11Timer::BufferSize);
	pContext->End(static_cast<DX11Timer*>(timer)->Queries[currFrameIdx].End.Get());
	if (mUserAnnotation)
	{
		mUserAnnotation->EndEvent();
	}
}
#endif

void DX11Renderer::SetPixelShader(PixelShader const* shader)
{
	DX11PixelShader* dx11Shader = shader ? static_cast<DX11PixelShader*>(shader->GetDeviceShader()) : nullptr;
	pContext->PSSetShader(dx11Shader ? dx11Shader->GetShader() : nullptr, nullptr, 0);
}

void DX11Renderer::SetVertexShader(VertexShader const* shader)
{
	mCurrVertexShader = shader;
	DX11VertexShader* dx11Shader = shader ? static_cast<DX11VertexShader*>(shader->GetDeviceShader()) : nullptr;
	pContext->VSSetShader(dx11Shader ? dx11Shader->GetShader() : nullptr, nullptr, 0);
	if (mCurrVertexLayoutHdl >= 0)
	{
		UpdateInputLayout();
	}
}

void DX11Renderer::SetComputeShader(ComputeShader const* shader)
{
	DX11ComputeShader* dx11Shader = shader ? static_cast<DX11ComputeShader*>(shader->GetDeviceShader()) : nullptr;
	pContext->CSSetShader(dx11Shader ? dx11Shader->GetShader() : nullptr, nullptr, 0);
}

void DX11Renderer::DrawMesh(Primitive const& primitive)
{
	auto& meshData = m_MeshData[&primitive];
	if (!meshData.vBuff)
	{
		PrepareMesh(primitive, meshData);
	}
	assert(meshData.iBuff);

	const UINT stride = Sizeof(primitive.vertices[0]);
	const UINT offset = 0;

	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->IASetVertexBuffers(0, 1, meshData.vBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(meshData.iBuff.Get(),DXGI_FORMAT_R16_UINT, 0);
	pContext->DrawIndexed(Size(primitive.triangles) * 3,0,0);
}

void DX11Renderer::SetVertexLayout(VertAttDescHandle attDescHandle)
{
	mCurrVertexLayoutHdl = attDescHandle;
	mCurrVertexLayout = &VertexAttributeDesc::GetRegistry().Get(attDescHandle);
	if (mCurrVertexShader)
	{
		UpdateInputLayout();
	}
}

void DX11Renderer::UpdateInputLayout()
{
	ID3D11InputLayout* inputLayout = GetOrCreateInputLayout(mCurrVertexLayoutHdl, mCurrVertexShader->GetInputSignature());
	pContext->IASetInputLayout(inputLayout);
}

void DX11Renderer::SetRTAndDS(IRenderTarget::Ref rt, IDepthStencil::Ref ds, int RTArrayIdx, int DSArrayIdx)
{
	auto*					dx11DS = static_cast<DX11DepthStencil*>(ds.get());
	ID3D11DepthStencilView* dsv= nullptr;
	if (dx11DS != nullptr) 
	{
		if (DSArrayIdx != -1)
		{
			dsv = dx11DS->GetDSV(DSArrayIdx);
		}
		else
		{
			dsv = dx11DS->GetDSV();
		}
	}

	ID3D11RenderTargetView* rtv= nullptr;
	auto*					dx11RT = static_cast<DX11RenderTarget*>(rt.get());
	if (dx11RT != nullptr)
	{
		if (RTArrayIdx != -1)
		{
			rtv = dx11RT->GetRTV(RTArrayIdx);
		}
		else
		{
			rtv = dx11RT->GetRTV();
		}
	}
	
	pContext->OMSetRenderTargets(1, &rtv, dsv);
}

void DX11Renderer::SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds)
{
	std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> renderTargets;
	ZE_ASSERT(rts.size() <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
	for (u32 i=0; i<rts.size(); ++i)
	{
		if (rts[i] != nullptr)
		{
			renderTargets[i] = static_cast<DX11RenderTarget*>(rts[i].get())->GetRTV();
		}
	}

	auto* dx11DS = static_cast<DX11DepthStencil*>(ds.get());
	ID3D11DepthStencilView* dsv = dx11DS ? dx11DS->GetDSV() : nullptr;

	pContext->OMSetRenderTargets(NumCast<u32>(rts.size()), &renderTargets[0], dsv);
}

void DX11Renderer::SetViewport(float width, float height, float TopLeftX /*= 0*/, float TopLeftY /*= 0*/)
{
	D3D11_VIEWPORT viewport = {
		.TopLeftX = TopLeftX,
		.TopLeftY = TopLeftY,
		.Width = float(width),
		.Height = float(height),
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	pContext->RSSetViewports(1, &viewport);
}

void DX11Renderer::SetDepthStencilMode(EDepthMode mode, StencilState stencil /*= EStencilMode::DISABLED*/)
{
	mStencilState = stencil;
	mDepthMode = mode;
	auto& state = m_DSStates[Denum(mode)][stencil.Mode];
	if (state == nullptr)
	{
		CreateDepthStencilState(state, mode, stencil);
	}
	pContext->OMSetDepthStencilState(state.Get(), stencil.WriteValue);
}

void DX11Renderer::SetDepthMode(EDepthMode mode)
{
	if (mode != mDepthMode)
	{
		SetDepthStencilMode(mode, mStencilState);
	}
}

void DX11Renderer::SetStencilState(StencilState state)
{
	SetDepthStencilMode(mDepthMode, state);
}

void DX11Renderer::CreateDepthStencilState(ComPtr<ID3D11DepthStencilState>& state, EDepthMode depth, StencilState stencil)
{
		
	CD3D11_DEPTH_STENCIL_DESC	 dsDesc {CD3D11_DEFAULT{}};
	if ((depth & EDepthMode::NoWrite) == EDepthMode::NoWrite)
	{
		depth ^= EDepthMode::NoWrite;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	}
	else
	{
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	}

	switch (depth)
	{
	case rnd::EDepthMode::Disabled:
	{
		dsDesc.DepthEnable = false;
		break;
	}
	case rnd::EDepthMode::LessEqual:
	{
		dsDesc.DepthEnable = true;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		break;
	}
	case rnd::EDepthMode::Equal:
	{
		dsDesc.DepthEnable = true;
		dsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
		break;
	}
	case EDepthMode::Less:
		dsDesc.DepthEnable = true;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		break;
	default:
		ZE_ENSURE(false);
		break;
	}

	if ((stencil.Mode & EStencilMode::Overwrite) == EStencilMode::Overwrite)
	{
		dsDesc.StencilEnable = true;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	}
	if ((stencil.Mode & EStencilMode::IgnoreDepth) == EStencilMode::IgnoreDepth)
	{
		dsDesc.StencilEnable = true;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	}
	if (!!(stencil.Mode & EStencilMode::UseBackFace))
	{
		dsDesc.BackFace = dsDesc.FrontFace;
	}

	HR_ERR_CHECK(pDevice->CreateDepthStencilState(&dsDesc, &state));

}


void DX11Renderer::SetBlendMode(EBlendState mode)
{
	ID3D11BlendState* blendState = nullptr;
	if (mode == (EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE))
	{
		blendState = nullptr;
	}
	else if (mode == (EBlendState::COL_ADD | EBlendState::ALPHA_MAX))
	{
		blendState = m_BlendState.Get();
	}
	else if (mode == (EBlendState::COL_BLEND_ALPHA | EBlendState::COL_OBSCURE_ALPHA | EBlendState::ALPHA_ADD))
	{
		blendState = m_AlphaBlendState.Get();
	}
	else if (mode == (EBlendState::COL_BLEND_ALPHA | EBlendState::ALPHA_UNTOUCHED))
	{
		blendState = m_AlphaLitBlendState.Get();
	}
	else
	{
		blendState = nullptr;
	}

	pContext->OMSetBlendState(blendState, nullptr, 0xffffffff);
}

void DX11Renderer::ClearDepthStencil(IDepthStencil::Ref ds, EDSClearMode clearMode, float depth, u8 stencil /* = 0 */)
{
	DX11DepthStencil* dx11DS = static_cast<DX11DepthStencil*>(ds.get());
	for (auto const& DSV : dx11DS->DepthStencils)
	{
		pContext->ClearDepthStencilView(DSV.Get(), static_cast<D3D11_CLEAR_FLAG>(clearMode), depth, stencil);
	}
}

void DX11Renderer::ClearRenderTarget(IRenderTarget::Ref rt, col4 clearColour)
{
	const float clearCol[4] = {clearColour.x, clearColour.y, clearColour.z, clearColour.w};
	pContext->ClearRenderTargetView(rt->GetData<ID3D11RenderTargetView*>(), clearCol);
}

void DX11Renderer::ClearUAV(UnorderedAccessView uav, vec4 clearValues)
{
	if (auto dxUav = uav.Get<ID3D11UnorderedAccessView*>())
	{
		pContext->ClearUnorderedAccessViewFloat(dxUav, &clearValues[0]);
	}
}

void DX11Renderer::ClearUAV(UnorderedAccessView uav, uint4 clearValues)
{

	if (auto dxUav = uav.Get<ID3D11UnorderedAccessView*>())
	{
		pContext->ClearUnorderedAccessViewUint(dxUav, &clearValues[0]);
	}
}

void DX11Renderer::SetConstantBuffers(EShaderType shader, Span<IConstantBuffer* const> buffers)
{
	constexpr u32 MAX_CONSTANT_BUFFERS = 14;
	ID3D11Buffer* dx11Buffers[MAX_CONSTANT_BUFFERS];
	u32 numBuffers = NumCast<u32>(buffers.size());
	ZE_ASSERT(numBuffers <= MAX_CONSTANT_BUFFERS);
	for (u32 i = 0; i < std::min(MAX_CONSTANT_BUFFERS, numBuffers); ++i)
	{
		if (!buffers[i])
		{
			dx11Buffers[i] = nullptr;
		}
		else
		{
			dx11Buffers[i] = static_cast<DX11ConstantBuffer*>(buffers[i])->GetDeviceBuffer();
		}
	}
	switch (shader)
	{
	case EShaderType::Vertex:
		pContext->VSSetConstantBuffers(0, numBuffers, dx11Buffers);
		break;
	case EShaderType::Pixel:
		pContext->PSSetConstantBuffers(0, numBuffers, dx11Buffers);
		break;
	case EShaderType::Compute:
		pContext->CSSetConstantBuffers(0, numBuffers, dx11Buffers);
		break;
	default:
		break;
	}
}

void DX11Renderer::SetConstantBuffers(EShaderType shaderType, std::span<CBHandle const> handles)
{
	constexpr u64 MAX_CONSTANT_BUFFERS = 16;
	ID3D11Buffer* dx11Buffers[MAX_CONSTANT_BUFFERS];
	for (u32 i = 0; i < std::min(MAX_CONSTANT_BUFFERS, handles.size()); ++i)
	{
		dx11Buffers[i] = handles[i].UserData.As<ID3D11Buffer*>();
	}
	switch (shaderType)
	{
	case EShaderType::Vertex:
		pContext->VSSetConstantBuffers(0, NumCast<u32>(handles.size()), dx11Buffers);
		break;
	case EShaderType::Pixel:
		pContext->PSSetConstantBuffers(0, NumCast<u32>(handles.size()), dx11Buffers);
		break;
	case EShaderType::Compute:
		pContext->CSSetConstantBuffers(0, NumCast<u32>(handles.size()), dx11Buffers);
		break;
	default:
		ZE_ASSERT(false);
		break;
	}
}

inline u32 GetSubresourceIdx(DeviceSubresource const& Subresource)
{
	return Subresource.MipIdx + (Subresource.ArrayIdx * Subresource.Resource->Desc.NumMips);
}

void DX11Renderer::ResolveMultisampled(DeviceSubresource const& Dest, DeviceSubresource const& Src)
{
	DXGI_FORMAT format = GetDxgiFormat(Dest.Resource->Desc.Format, ETextureFormatContext::RenderTarget);
	ID3D11Resource* dst = Dest.Resource->GetData<ID3D11Resource*>();
	ID3D11Resource* src = Src.Resource->GetData<ID3D11Resource*>();
	u32				dstIdx = GetSubresourceIdx(Dest);
	u32				srcIdx = GetSubresourceIdx(Src);
	pContext->ResolveSubresource(dst, dstIdx, src, srcIdx, format);
}

void DX11Renderer::UpdateConstantBuffer(CBHandle handle, std::span<const byte> data)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Zero(mappedResource);
	
	ID3D11Buffer* buff = handle.UserData.As<ID3D11Buffer*>();
	HR_ERR_CHECK(pContext->Map(buff, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, &data[0], data.size());
	pContext->Unmap(buff, 0);
}

struct ShaderVariant
{
	ShaderVariant(String const& name, Vector<D3D_SHADER_MACRO>&& defines)
		: m_Name(name), m_Defines(std::move(defines))
	{
		m_Defines.push_back({nullptr, nullptr});
	}
	String m_Name;
	Vector<D3D_SHADER_MACRO> m_Defines;
	ComPtr<ID3DBlob> m_Blob;
	bool m_Dirty = false;
};

u64 Hash(ID3DBlob* blob)
{
	size_t size = blob->GetBufferSize();
	u64 result = 0;
	size_t u64Size = size/8;
	for (u32 i=0; i<u64Size; ++i)
	{
		result ^= *(reinterpret_cast<const u64*>(blob->GetBufferPointer()) + i);
	}
	u32 finalOffset = 0;
	for (u32 i=0; i<size%8; ++i)
	{
		result ^= (u64(*(reinterpret_cast<const u8*>(blob->GetBufferPointer()) + u64Size * 4 + i)) << finalOffset);
		finalOffset++;
	}
	return result;
}


int GetCompiledShaderVariants(const String& shaderName, char const* name, ShaderVariant* variants, u32 numVars, char const* shaderType,
	bool reload = false)
{
	static fs::path const srcDir{ "src\\shaders" };
	static fs::path const outDir{ "generated\\shaders" };
	fs::create_directories(outDir);
	auto srcName = std::format("{}.hlsl", name);
	fs::path src = srcDir / srcName;
	auto lastWrite = fs::last_write_time(src);
	ComPtr<ID3DBlob> errBlob;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
//	flags |= D3DCOMPILE_DEBUG;
#endif
	for (u32 i=0; i<numVars; ++i)
	{
		ShaderVariant& var = variants[i];
		auto csoName = std::format("{}_{}.cso", shaderName, var.m_Name);
		fs::path cso = outDir / csoName;
		if (reload || var.m_Dirty || !fs::exists(cso) || fs::last_write_time(cso) < lastWrite)
		{
			printf("Compiling shader in file %s\n", src.string().c_str());
			ID3DBlob* output;
			HRESULT hr = D3DCompileFromFile(src.wstring().c_str(), Addr(var.m_Defines), D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderType, flags, 0, &output, &errBlob);
			if (SUCCEEDED(hr))
			{
				if (reload && var.m_Blob)
				{
					if(output->GetBufferSize() != var.m_Blob->GetBufferSize() ||
						memcmp(output->GetBufferPointer(), var.m_Blob->GetBufferPointer(), output->GetBufferSize()) != 0)
					{
						fprintf(stderr, "%s HAS CHANGED (hash before %llu, hash after %llu)\n", cso.string().c_str(), Hash(var.m_Blob.Get()), Hash(output));
					}
				}
				var.m_Blob = output;
				printf("Saving to file %s\n", cso.string().c_str());
				HR_ERR_CHECK(D3DWriteBlobToFile(var.m_Blob.Get(), cso.c_str(), true));
//				#define BLOB_DEBUG
				#ifdef BLOB_DEBUG
				{
					ComPtr<ID3DBlob> saved;
					HR_ERR_CHECK(D3DReadFileToBlob(cso.c_str(), &saved));
					ZE_ASSERT(saved->GetBufferSize() == var.m_Blob->GetBufferSize() &&
						memcmp(saved->GetBufferPointer(), var.m_Blob->GetBufferPointer(), saved->GetBufferSize()) == 0);
					printf("Hash: %llu\n", Hash(var.m_Blob.Get()));

				}
				#endif
			}
			else
			{
				LPTSTR errorText = NULL;
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
							| FORMAT_MESSAGE_ALLOCATE_BUFFER
							| FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL, hr, 0, (LPTSTR) & errorText, 0, NULL);
				wprintf(L"Compile error: %s", errorText);
				if (errBlob != nullptr && errBlob->GetBufferPointer() != nullptr)
				{
					printf("Output: %s\n", (const char*) errBlob->GetBufferPointer());
				}
				ZE_ASSERT(false);
				return i;
			}
		}
		else
		{
			printf("Reading precompiled shader %s from %s\n", name, cso.string().c_str());
			HR_ERR_CHECK(D3DReadFileToBlob(cso.c_str(), &var.m_Blob));
			printf("Hash: %llu\n", Hash(var.m_Blob.Get()));
		}
	}
	return numVars;
}


void GetCBInfo(ID3DBlob* shaderCode, DX11MaterialType& matType, ECBFrequency perFrame, ECBFrequency perInst)
{
	ComPtr<ID3D11ShaderReflection> reflection;
	HR_ERR_CHECK(D3DReflect(shaderCode->GetBufferPointer(), shaderCode->GetBufferSize(), IID_ID3D11ShaderReflection, &reflection))
	D3D11_SHADER_DESC desc;
	reflection->GetDesc(&desc);
	for (u32 i = 0; i < desc.ConstantBuffers; ++i)
	{
		ID3D11ShaderReflectionConstantBuffer* cBuffer = reflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC			  cbDesc;
		cBuffer->GetDesc(&cbDesc);
		ECBFrequency freq = ECBFrequency::Count;
		printf("Found cb %s\n", cbDesc.Name);
		if (FindIgnoreCase(cbDesc.Name, "instance"))
		{
			freq = perInst;
		}
		else if (FindIgnoreCase(cbDesc.Name, "frame"))
		{
			freq = perFrame;
		}

		if (freq != ECBFrequency::Count)
		{
			matType.GetCBData(freq).IsUsed = true;
		}
	}
}


DEFINE_MATERIAL_SHADER(TexturedMat, "TexVertexShader", "main", "TexPixelShader", "main");
DEFINE_MATERIAL_SHADER(PlainMat, "PlainVertexShader", "main", "PlainPixelShader", "main");

void DX11Renderer::LoadShaders(bool reload)
{
	{
		const D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "Tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uvs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		CreateMatType2("Plain", MAT_PLAIN, "PlainVertexShader", PlainMat, ied, Size(ied), reload);
		CreateMatType2("Textured", MAT_TEX, "TexVertexShader", TexturedMat, ied, Size(ied), reload);
		CreateMatTypeUnshaded("PointShadow", MAT_POINT_SHADOW_DEPTH, "PointShadow_VS", "PointShadow_PS", ied, Size(ied), reload);
		CreateMatTypeUnshaded("ScreenId", MAT_SCREEN_ID, "PlainVertexShader", "ScreenId_PS", ied, Size(ied), reload, { D3D_SHADER_MACRO{ "SHADED", "0" } });
		static DataLayout screenIdLayout(16, { { &GetTypeInfo<u32>(), "screenObjectId", 0 } });
		m_MatTypes[MAT_SCREEN_ID].CBData[Denum(ECBFrequency::PS_PerInstance)].Layout = &screenIdLayout;
	}

	{
		const D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FlatVert, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		CreateMatTypeUnshaded("2D", MAT_2D, "2D_VS", "2D_PS", ied, Size(ied), reload);
		CreateMatTypeUnshaded("2D_Uint", MAT_2D_UINT, "2D_VS", "2D_PS_Uint", ied, Size(ied), reload);
		CreateMatTypeUnshaded("Cubemap", MAT_BG, "BGVertexShader", "BGPixelShader", ied, Size(ied), reload);
		CreateMatTypeUnshaded("Cubedepth", MAT_CUBE_DEPTH, "BGVertexShader", "BGPixelShader", ied, Size(ied), reload, { D3D_SHADER_MACRO{ "DEPTH_SAMPLE", "1" } });
	
	}
}

void DX11Renderer::CreateMatType2(String const& name, u32 index, char const* vsName, const MaterialArchetypeDesc& typeDesc, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload)
{

	DX11MaterialType& matType = m_MatTypes[index];
	matType.CBData[Denum(ECBFrequency::PS_PerInstance)].Layout = GetLayout<PerInstancePSData>();
	matType.DebugName = name;
	ShaderVariant variant = {"vertex", {}};
	if (GetCompiledShaderVariants(name + "_VS", vsName, &variant, 1, "vs_5_0", reload) > 0)
	{
		auto const& vertBlob = variant.m_Blob;
		GetCBInfo(vertBlob.Get(), matType, ECBFrequency::VS_PerFrame, ECBFrequency::VS_PerInstance);
		HR_ERR_CHECK(pDevice->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), nullptr, &matType.m_VertexShader));
		HR_ERR_CHECK(pDevice->CreateInputLayout(ied, iedsize, vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), &m_MatTypes[index].m_InputLayout));
		SetResourceName(m_MatTypes[index].m_VertexShader, vsName);
	}
	else
	{
		assert(false);
	}
	matType.Desc = typeDesc;
	//ComPtr<ID3DBlob> pixBlob;
	//HR_ERR_CHECK(D3DReadFileToBlob(psName, &pixBlob));
}

void DX11Renderer::CreateMatType(const String& name, u32 index, char const* vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload)
{
	ShaderVariant variants[] = {
		{ "base", {
			D3D_SHADER_MACRO{ "BASE_LAYER", "1" },
		}},
		{ "dirlight", {
			D3D_SHADER_MACRO{ "DIR_LIGHT", "1" },
			D3D_SHADER_MACRO{ "BASE_LAYER", "0" },
		}},
		{ "pointlight", {
			D3D_SHADER_MACRO{ "POINT_LIGHT", "1" },
			D3D_SHADER_MACRO{ "BASE_LAYER", "0" },
		}},
		{ "spotlight", {
			D3D_SHADER_MACRO{ "SPOTLIGHT", "1" },
			D3D_SHADER_MACRO{ "BASE_LAYER", "0" },
		}},
	};

	DX11MaterialType& matType = m_MatTypes[index];
	matType.CBData[Denum(ECBFrequency::PS_PerInstance)].Layout = GetLayout<PerInstancePSData>();
	matType.DebugName = psName;
	if (GetCompiledShaderVariants(name + "_VS", vsName, variants, Size(variants), "vs_5_0", reload) > 0)
	{
		auto const& vertBlob = variants[0].m_Blob;
		GetCBInfo(vertBlob.Get(), matType, ECBFrequency::VS_PerFrame, ECBFrequency::VS_PerInstance);
		HR_ERR_CHECK(pDevice->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), nullptr, &matType.m_VertexShader));
		HR_ERR_CHECK(pDevice->CreateInputLayout(ied, iedsize, vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), &m_MatTypes[index].m_InputLayout));
		SetResourceName(m_MatTypes[index].m_VertexShader, vsName);
	}
	else
	{
		assert(false);
	}
	//ComPtr<ID3DBlob> pixBlob;
	//HR_ERR_CHECK(D3DReadFileToBlob(psName, &pixBlob));
	int count = GetCompiledShaderVariants(name + "_PS", psName, variants, Size(variants), "ps_5_0", reload);
	for (int i=0; i<count; ++i )
	{
		auto const& pixBlob = variants[i].m_Blob;
		HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader[i]));
		SetResourceName(m_MatTypes[index].m_PixelShader[i], String(psName));
		if (i == 0)
		{
			GetCBInfo(pixBlob.Get(), matType, ECBFrequency::PS_PerFrame, ECBFrequency::PS_PerInstance);
		}
	}

}
//template<>
//struct std::hash<D3D_SHADER_MACRO>
//{
//	size_t operator()(const D3D)
//};

void DX11Renderer::CreateMatTypeUnshaded(const String& name, u32 index, char const* vsName, char const* psName,
										 const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload,
										 Vector<D3D_SHADER_MACRO>&& defines /* = {}  */)
{
	ShaderVariant variants[] = {
		{ "base", std::move(defines) },
	};

	DX11MaterialType& matType = m_MatTypes[index];
	if (GetCompiledShaderVariants(name + "_VS", vsName, variants, Size(variants), "vs_5_0", reload) > 0)
	{
		auto const& vertBlob = variants[0].m_Blob;
		GetCBInfo(vertBlob.Get(), matType, ECBFrequency::VS_PerFrame, ECBFrequency::VS_PerInstance);
		HR_ERR_CHECK(pDevice->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_VertexShader));
		HR_ERR_CHECK(pDevice->CreateInputLayout(ied, iedsize, vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), &m_MatTypes[index].m_InputLayout));
		SetResourceName(m_MatTypes[index].m_VertexShader, vsName);
	}
	else
	{
		assert(false);
	}
	//ComPtr<ID3DBlob> pixBlob;
	//HR_ERR_CHECK(D3DReadFileToBlob(psName, &pixBlob));
	int count = GetCompiledShaderVariants(name + "_PS", psName, variants, Size(variants), "ps_5_0", reload);
	for (int i=0; i<count; ++i )
	{
		auto const& pixBlob = variants[i].m_Blob;
		HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader[i]));
		SetResourceName(m_MatTypes[index].m_PixelShader[i], String(psName));
		if (i == 0)
		{
			GetCBInfo(pixBlob.Get(), matType, ECBFrequency::PS_PerFrame, ECBFrequency::PS_PerInstance);
		}
	}

}

DEFINE_CLASS_TYPEINFO(PerInstancePSData)
BEGIN_REFL_PROPS()
REFL_PROP(colour)
REFL_PROP(ambdiffspec)
REFL_PROP(roughness)
REFL_PROP(emissiveColour)
REFL_PROP(alphaMask)
REFL_PROP(metalness)
REFL_PROP(useNormalMap)
REFL_PROP(useEmissiveMap)
REFL_PROP(useRoughnessMap)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

}
}

DEFINE_CLASS_TYPEINFO(rnd::dx11::PerFrameVertexData)
BEGIN_REFL_PROPS()
REFL_PROP(screen2World)
REFL_PROP(world2Light)
REFL_PROP(cameraPos)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO(rnd::dx11::PerInstanceVSData)
BEGIN_REFL_PROPS()
REFL_PROP(fullTransform)
REFL_PROP(model2ShadeSpace)
REFL_PROP(model2ShadeDual)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

