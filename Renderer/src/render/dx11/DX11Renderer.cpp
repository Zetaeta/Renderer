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


using DX11Bool = int;

struct BGVert
{
	vec3 pos;
	vec2 uv;
};


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
	DX11Bool isDepth = false;
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

DEFINE_CLASS_TYPEINFO(PerFrameVertexData)
BEGIN_REFL_PROPS()
REFL_PROP(screen2World)
REFL_PROP(world2Light)
REFL_PROP(cameraPos)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO(PerInstanceVSData)
BEGIN_REFL_PROPS()
REFL_PROP(fullTransform)
REFL_PROP(model2ShadeSpace)
REFL_PROP(model2ShadeDual)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

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


void DX11Renderer::RenderShadowMap()
{
	SetDepthMode(EDepthMode::LESS);
	m_Ctx.psTextures.UnBind(m_Ctx);
	// Spotlights
	m_SpotLightShadows.resize(m_Scene->m_SpotLights.size());
	for (int i = 0; i < m_SpotLightShadows.size(); ++i)
	{
		auto& shadowMap = m_SpotLightShadows[i];
		if (!shadowMap)
		{
			shadowMap.Init(m_Ctx, 1024);
		}

		auto const& light = m_Scene->m_SpotLights[i];

		float const FAR_PLANE = 1000;
		float const NEAR_PLANE = .1f;
		mat4 projection =
		{ 
			{ 1/ light.tangle, 0, 0, 0 },
			{0,	1/ light.tangle, 0,0},
			{ 0, 0, (FAR_PLANE) / (FAR_PLANE - NEAR_PLANE), -FAR_PLANE * NEAR_PLANE  / (FAR_PLANE - NEAR_PLANE)},
			{0,			0,		1,0},
		};
		projection = glm::transpose(projection);
		QuatTransform w2ltrans;
		if (light.comp)
			w2ltrans = m_Scene->m_SpotLights[i].comp->GetWorldTransform();
		w2ltrans.SetScale(vec3(1));
		RenderDepthOnly(shadowMap.GetDSV(), 1024, 1024, glm::inverse(mat4(w2ltrans)), projection, nullptr, &shadowMap.m_World2LightProj);
	}

	// Directional
	m_DirLightShadows.resize(m_Scene->m_DirLights.size());
	for (int i = 0; i < m_DirLightShadows.size(); ++i)
	{
		auto& shadowMap = m_DirLightShadows[i];
		if (!shadowMap)
		{
			shadowMap.Init(m_Ctx, 1024);
		}

		auto const& light = m_Scene->m_DirLights[i];

		float const FAR_PLANE = 1000;
		float const NEAR_PLANE = .1f;
		mat4 projection =
		{ 
			{ 1.f/m_DirShadowSize, 0, 0, 0 },
			{0,	1.f/m_DirShadowSize, 0,0},
			{ 0, 0, 1 / (FAR_PLANE - NEAR_PLANE), -NEAR_PLANE  / (FAR_PLANE - NEAR_PLANE)},
			{0,			0,		0,1},
		};

		projection = glm::transpose(projection);
		QuatTransform w2ltrans;
		if (light.comp)
			w2ltrans = light.comp->GetWorldTransform();
		w2ltrans.SetScale(vec3(1));
		w2ltrans.SetTranslation(w2ltrans.GetTranslation() - light.dir * 100.f);
		RenderDepthOnly(shadowMap.GetDSV(), 1024, 1024, glm::inverse(mat4(w2ltrans)), projection, nullptr, &shadowMap.m_World2LightProj);
	}

	// Point lights
	m_PointLightShadows.resize(m_Scene->m_PointLights.size());
	for (int i = 0; i < m_PointLightShadows.size(); ++i)
	{
		auto& shadowMap = m_PointLightShadows[i];
		if (!shadowMap)
		{
			shadowMap.Init(m_Ctx, 1024);
		}

		auto const& light = m_Scene->m_PointLights[i];

		float const FAR_PLANE = 1000;
		float const NEAR_PLANE = .1f;
		mat4 projection =
		{ 
			{ 1, 0, 0, 0 },
			{0,	1, 0,0},
			{ 0, 0, (FAR_PLANE) / (FAR_PLANE - NEAR_PLANE), -FAR_PLANE * NEAR_PLANE  / (FAR_PLANE - NEAR_PLANE)},
			{0,			0,		1,0},
		};
		projection = glm::transpose(projection);
		//glm::rotat
		QuatTransform l2wTrans;
		if (light.comp)
			l2wTrans = m_Scene->m_PointLights[i].comp->GetWorldTransform();
		l2wTrans.SetScale(vec3(1));

		mat4 world2Light = glm::inverse(mat4(l2wTrans));
		shadowMap.m_World2Light = l2wTrans;

		constexpr float ROTVAL = float(M_PI_2);
		static mat4 rots[] = {
			glm::rotate(ROTVAL,vec3{0,-1,0}), // +Z -> +X
			glm::rotate(ROTVAL,vec3{0,1,0}), // +Z -> -X
			glm::rotate(ROTVAL,vec3{1,0,0}), // +Z -> +Y
			glm::rotate(ROTVAL,vec3{-1,0,0}), // +Z -> +Y
			glm::identity<mat4>(),			   // +Z -> +Z
			glm::rotate(ROTVAL * 2.f,vec3{0,1,0}), // +Z -> -Z
		};

		PerFrameVertexData PFVD;
		PFVD.cameraPos = m_Camera->GetPosition();
		PFVD.world2Light = world2Light;
		m_VSPerFrameBuff.WriteData(PFVD);
		//float maxDepth = FAR_PLANE
		m_PSPerFrameBuff.WriteData(FAR_PLANE);
		//pContext->PSSetConstantBuffers(0, 1, m_PSPerFrameBuff.GetAddressOf());


		for (u32 i=0; i<6; ++i)
		{
			mat4 myProj= projection * rots[i];
			RenderDepthOnly(shadowMap.GetDSV(i), 1024, 1024, world2Light,
				myProj, &m_MatTypes[MAT_POINT_SHADOW_DEPTH], nullptr);
		}
	}
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


void DX11Renderer::RenderDepthOnly(ID3D11DepthStencilView *dsv, u32 width, u32 height,
	mat4 const& transform, mat4 const& projection, DX11MaterialType* material, mat4* outFullTrans)
{
	pContext->PSSetShaderResources(0,0, nullptr);
	pContext->OMSetRenderTargets(0, nullptr, dsv);
	pContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.f, 0);
	m_Ctx.psTextures.ClearFlags(E_TS_SHADOWMAP);

	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = float(width),
		.Height = float(height),
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	pContext->RSSetViewports(1, &viewport);


	float const FAR_PLANE = 1000;
	float const NEAR_PLANE = .1f;
	mat4 projWorld = projection * transform;
	if (outFullTrans)
	{
		*outFullTrans = projWorld; 
	}
	if (material != nullptr)
	{
		material->Bind(m_Ctx, EShadingLayer::BASE);
	}

	ForEachMesh([&](MeshPart const& mesh, Transform const& trans)
	{
		if (m_Scene->GetMaterial(mesh.material).translucent)
		{
			return;
		}
		PerInstanceVSData PIVS;
		PIVS.model2ShadeSpace = mat4(trans);
		PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
		PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

		m_VSPerInstanceBuff.WriteData(PIVS);

		DrawMesh(mesh, EShadingLayer::BASE, material == nullptr);
	});

	//for (auto const& mi : m_Scene->m_MeshInstances)
	//{
	//	if (mi.mesh == -1)
	//	{
	//		printf("Mesh instance with no mesh");
	//		continue;
	//	}
	//	Transform const& trans = mi.trans;
	//	Mesh const&		 mesh = m_Scene->GetMesh(mi.mesh);

	//	PerInstanceVSData PIVS;
	//	PIVS.model2ShadeSpace = mat4(trans);
	//	PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
	//	PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

	//	WriteCBuffer(m_VSPerInstanceBuff,PIVS);

	//	ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
	//	pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
	//	DrawMesh(mi.mesh, EShadingLayer::NONE, material == nullptr);
	//}
}

__declspec(align(16))
struct PerInstancePSData
{
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

 DX11Renderer::DX11Renderer(UserCamera* camera, u32 width, u32 height, ID3D11Device* device, ID3D11DeviceContext* context)
	: m_Height(height), m_Width(width), m_Camera(camera), m_PixelWidth(width), pDevice(device), pContext(context)
{
	m_Ctx.pDevice = pDevice;
	m_Ctx.pContext = pContext;
	m_Ctx.m_Renderer = this;
	Setup();
}

DX11Renderer::~DX11Renderer()
{
	m_Materials.clear();
	m_PointLightShadows.clear();
	m_DirLightShadows.clear();
	m_SpotLightShadows.clear();
	m_Ctx.psTextures = {};
	m_BG = nullptr;
	mRCtx = {};
 }

void DX11Renderer::DrawControls()
{
	ImGui::Begin("Renderer");
	ImGui::Checkbox("Refactor", &mUsePasses);
	ImGui::Checkbox("Pre-refactor shadows", &mRenderShadowMap);
	static const char* layers[] = { "base",
		"dirlight",
		"pointlight",
		"spotlight",
	};
	for (int i=0; i<Denum(EShadingLayer::NONE); ++i)
	{
		ImGui::Checkbox(layers[i], &m_Ctx.mRCtx->mLayersEnabled[i]);
	}
	if (ImGui::Button("Reload shaders"))
	{
		LoadShaders();
	}
	if (ImGui::Button("Full recompile shaders"))
	{
		LoadShaders(true);
	}

	ImGui::DragFloat("Dir shadowmap scale", &m_DirShadowSize);
	ImGui::DragFloat("Point shadow factor", &m_PointShadowFactor, 0.1f);
	ImGui::DragInt("Debug mode", reinterpret_cast<int*>(&mRCtx->ShadingDebugMode), 0.5, 0, Denum(EShadingDebugMode::COUNT));
	ImGui::DragFloat("Debug greyscale exponent", &mRCtx->mDebugGrayscaleExp, 0.1, 0, 10);
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

		if (m_ViewerTex != nullptr)
		{
			ImGui::Image(m_ViewerTex->GetSRV(), {500, 500}, {0,1}, {1,0});
		}
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
	m_EmptyTexture = DX11Texture::Create(&m_Ctx, 1, 1, &empty);

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

	{
		CD3D11_DEPTH_STENCIL_DESC	 dsDesc {CD3D11_DEFAULT{}};
//		D3D11_DEPTH_STENCIL_DESC dsDesc;
//		Zero(dsDesc);
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
		pDevice->CreateDepthStencilState(&dsDesc, &m_DSStates[Denum(EDepthMode::EQUAL)]);
	}
	{
		CD3D11_DEPTH_STENCIL_DESC	 dsDesc {CD3D11_DEFAULT{}};
//		D3D11_DEPTH_STENCIL_DESC dsDesc;
//		Zero(dsDesc);
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		pDevice->CreateDepthStencilState(&dsDesc, &m_DSStates[Denum(EDepthMode::LESS_EQUAL)]);
	}
	{
		CD3D11_DEPTH_STENCIL_DESC	 dsDesc {CD3D11_DEFAULT{}};
		dsDesc.DepthEnable = false;
		pDevice->CreateDepthStencilState(&dsDesc, &m_DSStates[Denum(EDepthMode::DISABLED)]);
	}

	// 2D
	{
		

		float const Z = -0.f;
		BGVert const verts[] = {
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
	BGVert const verts[3] = {
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
	ID3D11SamplerState* samplers[] = { m_Sampler.Get(), m_ShadowSampler.Get() };

	pContext->PSSetSamplers(0, 2, samplers );
	m_Scene = const_cast<Scene*>(&scene);
	m_Materials.resize(scene.Materials().size());

	if (mRenderShadowMap)
	{
		RenderShadowMap();
	}
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	auto   RTV = m_MainRenderTarget->GetRTV();
	pContext->OMSetRenderTargets(1, &RTV, NULL_OR(m_MainDepthStencil, GetDSV()));

	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	pContext->ClearRenderTargetView(m_MainRenderTarget->GetRTV(), clear_color_with_alpha);
	pContext->ClearDepthStencilView(m_MainDepthStencil->GetDSV(), D3D11_CLEAR_DEPTH, 1.f, 0);

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
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::BASE)])
		{
			Render(scene, EShadingLayer::BASE, -1, false);
		}
		pContext->OMSetBlendState(m_BlendState.Get(), nullptr, 0xffffffff);
		//pContext->OMSetDepthStencilState(m_LightsDSState.Get(), 1);
		SetDepthMode(EDepthMode::EQUAL);

		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::DIRLIGHT)])
		{
			for (int i = 0; i < scene.m_DirLights.size(); ++i)
			{
				Render(scene, EShadingLayer::DIRLIGHT, i, false);
			}
		}
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::SPOTLIGHT)])
		{
			for (int i = 0; i < scene.m_SpotLights.size(); ++i)
			{
				Render(scene, EShadingLayer::SPOTLIGHT, i,false);
			}
		}
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::POINTLIGHT)])
		{
			for (int i = 0; i < scene.m_PointLights.size(); ++i)
			{
				Render(scene, EShadingLayer::POINTLIGHT, i,false);
			}
		}

		pContext->OMSetBlendState(m_AlphaBlendState.Get(), nullptr, 0xffffffff);
	//	pContext->OMSetDepthStencilState(m_AlphaDSState.Get(), 1);
		SetDepthMode(EDepthMode::LESS_EQUAL);
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::BASE)])
		{
			Render(scene, EShadingLayer::BASE, -1, true);
		}

		pContext->OMSetBlendState(m_AlphaLitBlendState.Get(), nullptr, 0xffffffff);
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::DIRLIGHT)])
		{
			for (int i = 0; i < scene.m_DirLights.size(); ++i)
			{
				Render(scene, EShadingLayer::DIRLIGHT, i, true);
			}
		}
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::SPOTLIGHT)])
		{
			for (int i = 0; i < scene.m_SpotLights.size(); ++i)
			{
				Render(scene, EShadingLayer::SPOTLIGHT, i, true);
			}
		}
		if (mRCtx->mLayersEnabled[Denum(EShadingLayer::POINTLIGHT)])
		{
			for (int i = 0; i < scene.m_PointLights.size(); ++i)
			{
				Render(scene, EShadingLayer::POINTLIGHT, i, true);
			}
		}
		pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	//	pContext->OMSetDepthStencilState(nullptr, 1);
		SetDepthMode(EDepthMode::LESS);
		DrawBG();
	}



	pContext->ClearDepthStencilView(m_MainDepthStencil->GetDSV(), D3D11_CLEAR_DEPTH, 1.f, 0);
	if (m_ViewerTex != nullptr)
	{
		DrawTexture(m_ViewerTex);
	}

	pContext->PSSetShader(nullptr, nullptr, 0);
	pContext->VSSetShader(nullptr, nullptr, 0);

	m_Ctx.psTextures.UnBind(m_Ctx);
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

		CreateMatType(MAT_PLAIN, "PlainVertexShader","PlainPixelShader",ied,Size(ied), reload);
		CreateMatType(MAT_TEX, "TexVertexShader","TexPixelShader",ied,Size(ied), reload);
		CreateMatTypeUnshaded(MAT_POINT_SHADOW_DEPTH, "PointShadow_VS", "PointShadow_PS", ied,Size(ied), reload);
	}

	{
		const D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(BGVert, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		CreateMatTypeUnshaded(MAT_2D, "2D_VS", "2D_PS", ied, Size(ied), reload);
		CreateMatTypeUnshaded(MAT_BG, "BGVertexShader", "BGPixelShader", ied, Size(ied), reload);
		CreateMatTypeUnshaded(MAT_CUBE_DEPTH, "BGVertexShader", "BGPixelShader", ied, Size(ied), reload, { D3D_SHADER_MACRO{ "DEPTH_SAMPLE", "1" } });
	
	}
}

void DX11Renderer::DrawBG()
{
	DrawCubemap(m_BG->GetSRV());
}

void DX11Renderer::DrawCubemap(ID3D11ShaderResourceView* srv, bool depth)
{
	const UINT stride = sizeof(BGVert);
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

void DX11Renderer::DrawTexture(DX11Texture* tex, ivec2 pos, ivec2 size )
{
	VS2DCBuff cbuff;
	vec2 texturePos = vec2(pos) / vec2 {m_Width, m_Height};
	cbuff.pos = texturePos;// vec2(texturePos.x * 2 - 1, 1 - 2 * texturePos.y);
	if (size.x <= 0)
		size.x = 500;
	if (size.y <= 0)
		size.y = 500;
	cbuff.size = vec2(size) / vec2 {m_Width, m_Height};

	m_VS2DCBuff.WriteData(cbuff);

	PS2DCBuff pbuff;
	pbuff.isDepth = tex->IsDepthStencil();
	pbuff.exponent = m_TexViewExp;
	m_PS2DCBuff.WriteData(pbuff);

	const UINT stride = sizeof(BGVert);
	const UINT offset = 0;

	pContext->VSSetConstantBuffers(0, 1, m_VS2DCBuff.GetAddressOf());
	pContext->PSSetConstantBuffers(0, 1, m_PS2DCBuff.GetAddressOf());

	pContext->IASetVertexBuffers(0, 1, m_2DVBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_MatTypes[MAT_2D].Bind(m_Ctx, EShadingLayer::BASE);

	auto* srv = tex->GetSRV();
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
	ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
	pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
	auto& meshData = m_MeshData[&mesh];
	if (!meshData.vBuff)
	{
		PrepareMesh(mesh, meshData);
	}
	assert(meshData.iBuff);

	if (!m_Materials[mesh.material] || m_Scene->GetMaterial(mesh.material).NeedsUpdate())
	{
		PrepareMaterial(mesh.material);
	}

	if (useMaterial)
	{
		m_Materials[mesh.material]->Bind(m_Ctx, layer);
	}

	PerInstancePSData PIPS;
	Material const& mat = m_Scene->GetMaterial(mesh.material);
	PIPS.colour = mat.colour;
	PIPS.emissiveColour = mat.emissiveColour;
	PIPS.roughness = mat.roughness;
	PIPS.metalness = mat.metalness;
	PIPS.ambdiffspec = {1, mat.diffuseness, mat.specularity};
	PIPS.useNormalMap = mat.normal->IsValid();
	PIPS.useEmissiveMap = mat.emissiveMap->IsValid();
	PIPS.useRoughnessMap = mat.roughnessMap->IsValid();
	PIPS.alphaMask = mat.mask;
	m_PSPerInstanceBuff.WriteData(PIPS);

	const UINT stride = Sizeof(mesh.vertices[0]);
	const UINT offset = 0;

	ID3D11Buffer* pbuffs[] = { m_PSPerFrameBuff.Get(), m_PSPerInstanceBuff.Get() };
	pContext->PSSetConstantBuffers(0, Size(pbuffs), pbuffs);

	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->IASetVertexBuffers(0, 1, meshData.vBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(meshData.iBuff.Get(),DXGI_FORMAT_R16_UINT, 0);
	pContext->DrawIndexed(Size(mesh.triangles) * 3,0,0);
	m_Materials[mesh.material]->UnBind(m_Ctx);
}

void DX11Renderer::PrepareMesh(MeshPart const& mesh, DX11Mesh& meshData)
{
	printf("Creating buffers for mesh %s\n", mesh.name.c_str());
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
}

void DX11Renderer::SetMainRenderTarget(ComPtr<ID3D11RenderTargetView> rt, ComPtr<ID3D11DepthStencilView> ds, u32 width, u32 height)
{
	if (m_MainRenderTarget == nullptr)
	{
		m_MainRenderTarget = std::make_shared<DX11RenderTarget>(RenderTargetDesc{ "MainRT", ETextureDimension::TEX_2D, width, height }, rt, ds);
		m_MainDepthStencil = std::make_shared<DX11DepthStencil>(DepthStencilDesc{ "MainDS",  ETextureDimension::TEX_2D, width, height }, ds);
		mRCtx = std::make_unique<RenderContext>(&m_Ctx, m_Camera, m_MainRenderTarget, m_MainDepthStencil);
		m_Ctx.mRCtx = mRCtx.get();
	}
	else
	{
		m_MainRenderTarget->RenderTargets = {rt};
		m_MainDepthStencil->DepthStencils = {ds};
	}
	m_MainRenderTarget->Desc.Width = width;
	m_MainRenderTarget->Desc.Height = height;
	m_MainDepthStencil->Desc.Width = width;
	m_MainDepthStencil->Desc.Height = height;

}

void DX11Renderer::Resize(u32 width, u32 height, u32* canvas)
{
	m_Width = width;
	m_Height = height;
	m_Dirty = true;
	m_Scale = float(min(width, height));
}

DX11Texture::Ref DX11Renderer::PrepareTexture(Texture const& tex, bool sRGB /*= false*/)
{
	if (IsValid(tex.GetDeviceTexture()))
	{
		return std::static_pointer_cast<DX11Texture>(tex.GetDeviceTexture());
	}
	auto result = DX11Texture::Create(&m_Ctx, tex.width, tex.height, tex.GetData(), sRGB ? TF_SRGB : TF_NONE);
	tex.SetDeviceTexture(result);
	return result;
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

void DX11Renderer::SetDepthMode(EDepthMode mode)
{
	pContext->OMSetDepthStencilState(m_DSStates[Denum(mode)].Get(), 0);
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
		RASSERT(false);
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

int GetCompiledShaderVariants(char const* name, ShaderVariant* variants, u32 numVars, char const* shaderType, bool reload = false)
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
		auto csoName = std::format("{}_{}.cso", name, var.m_Name);
		fs::path cso = outDir / csoName;
		if (reload || var.m_Dirty || !fs::exists(cso) || fs::last_write_time(cso) < lastWrite)
		{
			printf("Compiling shader in file %s\n", src.string().c_str());
			ID3DBlob* output;
			HRESULT hr = D3DCompileFromFile(src.wstring().c_str(), Addr(var.m_Defines), D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderType, flags, 0, &output, &errBlob);
			if (SUCCEEDED(hr))
			{
				var.m_Blob = output;
				printf("Saving to file %s\n", cso.string().c_str());
				HR_ERR_CHECK(D3DWriteBlobToFile(var.m_Blob.Get(), cso.wstring().c_str(), true));
			}
			else
			{
				LPTSTR errorText = NULL;
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, 0, (LPTSTR) & errorText, 0, NULL);
				wprintf(L"Compile error: %s", errorText);
				if (errBlob != nullptr && errBlob->GetBufferPointer() != nullptr)
				{
					printf("Output: %s\n", (const char*) errBlob->GetBufferPointer());
				}
				return i;
			}
		}
		else
		{
			printf("Reading precompiled shader %s from %s\n", name, cso.string().c_str());
			HR_ERR_CHECK(D3DReadFileToBlob(cso.c_str(), &var.m_Blob));
		}
	}
	return numVars;
}

void DX11Renderer::CreateMatType(u32 index, char const* vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload)
{
	ShaderVariant variants[] = {
		{ "base", {
			D3D_SHADER_MACRO{ "BASE_LAYER", "1" },
		}},
		{ "dirlight", {
			D3D_SHADER_MACRO{ "DIR_LIGHT", "1" },
		}},
		{ "pointlight", {
			D3D_SHADER_MACRO{ "POINT_LIGHT", "1" },
		}},
		{ "spotlight", {
			D3D_SHADER_MACRO{ "SPOTLIGHT", "1" },
		}},
	};

	if (GetCompiledShaderVariants(vsName, variants, Size(variants), "vs_5_0", reload) > 0)
	{
		auto const& vertBlob = variants[0].m_Blob;
		HR_ERR_CHECK(pDevice->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_VertexShader));
		HR_ERR_CHECK(pDevice->CreateInputLayout(ied, iedsize, vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), &m_MatTypes[index].m_InputLayout));
	}
	else
	{
		assert(false);
	}
	//ComPtr<ID3DBlob> pixBlob;
	//HR_ERR_CHECK(D3DReadFileToBlob(psName, &pixBlob));
	int count = GetCompiledShaderVariants(psName, variants, Size(variants), "ps_5_0", reload);
	for (int i=0; i<count; ++i )
	{
		auto const& pixBlob = variants[i].m_Blob;
		HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader[i]));
	}

}

void DX11Renderer::CreateMatTypeUnshaded(u32 index, char const* vsName, char const* psName,
										 const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize, bool reload,
										 Vector<D3D_SHADER_MACRO>&& defines /* = {}  */)
{
	ShaderVariant variants[] = {
		{ "base", std::move(defines) },
	};

	if (GetCompiledShaderVariants(vsName, variants, Size(variants), "vs_5_0", reload) > 0)
	{
		auto const& vertBlob = variants[0].m_Blob;
		HR_ERR_CHECK(pDevice->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_VertexShader));
		HR_ERR_CHECK(pDevice->CreateInputLayout(ied, iedsize, vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), &m_MatTypes[index].m_InputLayout));
	}
	else
	{
		assert(false);
	}
	//ComPtr<ID3DBlob> pixBlob;
	//HR_ERR_CHECK(D3DReadFileToBlob(psName, &pixBlob));
	int count = GetCompiledShaderVariants(psName, variants, Size(variants), "ps_5_0", reload);
	for (int i=0; i<count; ++i )
	{
		auto const& pixBlob = variants[i].m_Blob;
		HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader[i]));
	}

}

