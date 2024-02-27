#include "DX11Renderer.h"
#include "Scene.h"
#include <functional>
#include <algorithm>
#include <format>
#include <filesystem>
#include "Camera.h"
#include <utility>
#include "DX11Backend.h"
#include <d3d11.h>
#include "WinUtils.h"
#include <d3dcompiler.h>
#include "CubemapDX11.h"
#include "SceneComponent.h"
#include "imgui.h"

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

using col = vec3; 


__declspec(align(16))
struct PerFramePSData
{
	col ambient;
	float _pad1;
};

__declspec(align(16))
struct PFPSDirLight// : PerFramePSData
{
	col directionalCol;
	float _pad2;
	vec3 directionalDir;
	float _pad3;
};

__declspec(align(16))
struct PFPSPointLight// : PerFramePSData
{
	vec3 pointLightPos;
	float pointLightRad;
	vec3 pointLightCol;
	float pointLightFalloff;
};

//__declspec(align(16))
struct PFPSSpotLight// : PerFramePSData
{
	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
	float _pad;
	mat4 world2Light;
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
	mat4 screen2World;
//	mat4 world2Light;
	vec3 cameraPos;
};

template <typename T>
void DX11Renderer::CreateConstantBuffer(ComPtr<ID3D11Buffer>& buffer, T const& initialData, u32 size)
{
	D3D11_BUFFER_DESC cbDesc;
	Zero(cbDesc);
	cbDesc.ByteWidth = size;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA srData;
	Zero(srData);
	srData.pSysMem = Addr(initialData);

	HR_ERR_CHECK(pDevice->CreateBuffer(&cbDesc, &srData,&buffer))
}

template <typename T>
void DX11Renderer::WriteCBuffer(ComPtr<ID3D11Buffer>& buffer, T const& data)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Zero(mappedResource);
	
	HR_ERR_CHECK(pContext->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, Addr(data), Sizeof(data));
	pContext->Unmap(buffer.Get(), 0);
}

void DX11Renderer::PrepareShadowMaps()
{
	//D3D11_TEXTURE2D_DESC shadowMapDesc;
	//ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	//shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	//shadowMapDesc.MipLevels = 1;
	//shadowMapDesc.ArraySize = 1;
	//shadowMapDesc.SampleDesc.Count = 1;
	//shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	//shadowMapDesc.Width = 1024;
	//shadowMapDesc.Height = 1024;

	//HR_ERR_CHECK(pDevice->CreateTexture2D(&shadowMapDesc, nullptr, &m_ShadowMap));

	//D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	//ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	//depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//depthStencilViewDesc.Texture2D.MipSlice = 0;

	//D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	//ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	//shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	//shaderResourceViewDesc.Texture2D.MipLevels = 1;

	//HR_ERR_CHECK(pDevice->CreateDepthStencilView(m_ShadowMap.Get(), &depthStencilViewDesc, &m_ShadowDepthView));
	//HR_ERR_CHECK(pDevice->CreateShaderResourceView(m_ShadowMap.Get(), &shaderResourceViewDesc, &m_ShadowSRV));

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
	comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;

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
	m_SpotLightShadows.resize(m_Scene->m_SpotLights.size());
	for (int i = 0; i < m_SpotLightShadows.size(); ++i)
	{
		auto& shadowMap = m_SpotLightShadows[i];
		if (shadowMap == nullptr)
		{
			shadowMap = std::make_unique<DX11ShadowMap>(m_Ctx);
		}
		pContext->PSSetShaderResources(0,0, nullptr);
		pContext->OMSetRenderTargets(0, nullptr, shadowMap->m_DepthView.Get());
		pContext->ClearDepthStencilView(shadowMap->m_DepthView.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);

		D3D11_VIEWPORT viewport = {
			.TopLeftX = 0,
			.TopLeftY = 0,
			.Width = 1024,
			.Height = 1024,
			.MinDepth = 0,
			.MaxDepth = 1,
		};
		pContext->RSSetViewports(1, &viewport);

		auto const& light = m_Scene->m_SpotLights[i];

		float const FAR_PLANE = 1000;
		float const NEAR_PLANE = .1f;
		m_Projection =
		{ 
			{ 1/ light.tangle, 0, 0, 0 },
			{0,	1/ light.tangle, 0,0},
			{ 0, 0, (FAR_PLANE) / (FAR_PLANE - NEAR_PLANE), -FAR_PLANE * NEAR_PLANE  / (FAR_PLANE - NEAR_PLANE)},
			{0,			0,		1,0},
		};
		m_Projection = glm::transpose(m_Projection);
		QuatTransform w2ltrans;
		if (light.comp)
			w2ltrans = m_Scene->m_SpotLights[i].comp->GetWorldTransform();
		w2ltrans.SetScale(vec3(1));
		mat4 light2World = w2ltrans;
		mat4 projWorld = m_Projection * inverse(light2World);
		shadowMap->m_World2LightProj = projWorld; 

		for (auto const& mi : m_Scene->m_MeshInstances)
		{
			if (mi.mesh == -1)
			{
				printf("Mesh instance with no mesh");
				continue;
			}
			Transform const& trans = mi.trans;
			Mesh const&		 mesh = m_Scene->GetMesh(mi.mesh);

			PerInstanceVSData PIVS;
			PIVS.model2ShadeSpace = mat4(trans);
			PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
			PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

			WriteCBuffer(m_VSPerInstanceBuff,PIVS);

			ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
			pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
			DrawMesh(mi.mesh, EShadingLayer::NONE);
		}
	}
}

__declspec(align(16))
struct PerInstancePSData
{
	ALIGN16(vec3, colour);
	vec3 ambdiffspec;
	int specularExp;
	DX11Bool useNormalMap;
	DX11Bool useEmissiveMap;
};

void DX11Renderer::DrawControls()
{
	static const char* layers[] = { "base",
		"dirlight",
		"pointlight",
		"spotlight",
	};
	for (int i=0; i<Denum(EShadingLayer::NONE); ++i)
	{
		ImGui::Checkbox(layers[i], &m_LayersEnabled[i]);
	}
	if (ImGui::Button("Reload shaders"))
	{
		LoadShaders();
	}
}

void DX11Renderer::Setup()
{
	//Setup matrix constant buffer
	CreateConstantBuffer<PerInstanceVSData>(m_VSPerInstanceBuff);
	CreateConstantBuffer<PerFrameVertexData>(m_VSPerFrameBuff);

	CreateConstantBuffer<PerInstancePSData>(m_PSPerInstanceBuff);
	CreateConstantBuffer<void*>(m_PSPerFrameBuff, nullptr, MaxSize<PFPSPointLight, PFPSSpotLight, PerFramePSData>());


	m_MatTypes.resize(MAT_COUNT);
	LoadShaders();
	//CreateMatType(MAT_PLAIN, "PlainVertexShader","PlainPixelShader",ied,Size(ied));
	//CreateMatType(MAT_TEX, "TexVertexShader","TexPixelShader",ied,Size(ied));

	u32 const empty = 0x00000000;
	m_EmptyTexture = ImageDX11::Create(pDevice, 1, 1, &empty);

	D3D11_SAMPLER_DESC sDesc;
	Zero(sDesc);
	sDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	HR_ERR_CHECK(pDevice->CreateSamplerState(&sDesc, &m_Sampler));
	pContext->PSSetSamplers(0, 1, m_Sampler.GetAddressOf());

	PrepareBG();
	PrepareShadowMaps();

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

	{
		CD3D11_DEPTH_STENCIL_DESC	 dsDesc {CD3D11_DEFAULT{}};
//		D3D11_DEPTH_STENCIL_DESC dsDesc;
//		Zero(dsDesc);
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
		pDevice->CreateDepthStencilState(&dsDesc, &m_LightsDSState);
	}

}
struct BGVert
{
	vec3 pos;
	vec2 uv;
};

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
	m_Background = ImageDX11::CreateFrom(tex, pDevice);
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(BGVert, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	
	CreateMatTypeUnshaded(MAT_BG, "BGVertexShader", "BGPixelShader", ied, Size(ied));
	m_BG = CubemapDX11::FoldUp(pDevice, tex);
}

void DX11Renderer::Render(const Scene& scene)
{
	m_Scene = &scene;
	m_MeshData.resize(scene.GetAssetManager()->GetMeshes().size());
	m_Materials.resize(scene.Materials().size());

	RenderShadowMap();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	pContext->OMSetRenderTargets(1, m_MainRenderTarget.GetAddressOf(), m_MainDepthStencil.Get());

	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	pContext->ClearRenderTargetView(m_MainRenderTarget.Get(), clear_color_with_alpha);
	pContext->ClearDepthStencilView(m_MainDepthStencil.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);


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

	D3D11_VIEWPORT vp = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = float(m_Width),
		.Height = float(m_Height),
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	pContext->RSSetViewports(1u, &vp);

	PerFrameVertexData PFVD;
	PFVD.cameraPos = m_Camera->position;
	PFVD.screen2World = inverse(projWorld);
	WriteCBuffer(m_VSPerFrameBuff, PFVD);

	m_PIVS.resize(scene.m_MeshInstances.size());
	for (u32 i=0; i< scene.m_MeshInstances.size(); ++i)
	{
		auto const& mi = scene.m_MeshInstances[i];
		if (mi.mesh == -1)
		{
			printf("Mesh instance with no mesh");
			continue;
		}
		Transform const& trans = mi.trans;
		Mesh const&		 mesh = scene.GetMesh(mi.mesh);

		
		PerInstanceVSData& PIVS = m_PIVS[i];
		PIVS.model2ShadeSpace = mat4(trans);
		PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
		PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

		WriteCBuffer(m_VSPerInstanceBuff,PIVS);

		//ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
		//pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
		//DrawMesh(mi.mesh);
	}
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	if (m_LayersEnabled[Denum(EShadingLayer::BASE)])
	{
		Render(scene, EShadingLayer::BASE);
	}
	pContext->OMSetBlendState(m_BlendState.Get(), nullptr, 0xffffffff);
	pContext->OMSetDepthStencilState(m_LightsDSState.Get(), 1);

	if (m_LayersEnabled[Denum(EShadingLayer::DIRLIGHT)])
	{
		for (int i = 0; i < scene.m_DirLights.size(); ++i)
		{
			Render(scene, EShadingLayer::DIRLIGHT, i);
		}
	}
	if (m_LayersEnabled[Denum(EShadingLayer::SPOTLIGHT)])
	{
		for (int i = 0; i < scene.m_SpotLights.size(); ++i)
		{
			Render(scene, EShadingLayer::SPOTLIGHT, i);
		}
	}
	if (m_LayersEnabled[Denum(EShadingLayer::POINTLIGHT)])
	{
		for (int i = 0; i < scene.m_PointLights.size(); ++i)
		{
			Render(scene, EShadingLayer::POINTLIGHT, i);
		}
	}


	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	pContext->OMSetDepthStencilState(nullptr, 1);
	DrawBG();

	pContext->PSSetShader(nullptr, nullptr, 0);
	pContext->VSSetShader(nullptr, nullptr, 0);

	m_Ctx.psTextures.UnBind(m_Ctx);
}


void DX11Renderer::Render(const Scene& scene, EShadingLayer layer, int index)
{

	switch (layer)
	{
	case EShadingLayer::BASE:
		PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(scene.m_AmbientLight);
		WriteCBuffer(m_PSPerFrameBuff, perFrameData);
		break;
	case EShadingLayer::DIRLIGHT:
	{
		auto const& light = scene.m_PointLights[index];
		PFPSDirLight perFrameData;
		Zero(perFrameData);
		perFrameData.directionalCol = vec3(scene.m_DirLights[index].colour);
		perFrameData.directionalDir = scene.m_DirLights[index].dir;
		WriteCBuffer(m_PSPerFrameBuff, perFrameData);
		break;
	}
		break;
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
		perFrameData.world2Light = m_SpotLightShadows[index]->m_World2LightProj;

		WriteCBuffer(m_PSPerFrameBuff, perFrameData);

		m_Ctx.psTextures.SetTexture(E_TS_SHADOWMAP, m_SpotLightShadows[index]->m_SRV.Get());
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
		WriteCBuffer(m_PSPerFrameBuff, perFrameData);
		break;
	}
	default:
		break;
	}
	for (u32 i=0; i< scene.m_MeshInstances.size(); ++i)
	{
		auto const& mi = scene.m_MeshInstances[i];
		if (mi.mesh == -1)
		{
			printf("Mesh instance with no mesh");
			continue;
		}

		WriteCBuffer(m_VSPerInstanceBuff, m_PIVS[i]);

		ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
		pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
		DrawMesh(mi.mesh, layer);
	}
}

void DX11Renderer::LoadShaders()
{
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uvs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	CreateMatType(MAT_PLAIN, "PlainVertexShader","PlainPixelShader",ied,Size(ied));
	CreateMatType(MAT_TEX, "TexVertexShader","TexPixelShader",ied,Size(ied));
}

void DX11Renderer::DrawBG()
{
	const UINT stride = sizeof(BGVert);
	const UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, m_BGVBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_MatTypes[MAT_BG].Bind(m_Ctx, EShadingLayer::BASE);
	//WriteCBuffer(m_VSPerFrameBuff)

//	ID3D11ShaderResourceView* srv = m_Background->GetSRV();
	pContext->PSSetShaderResources(0, 1, m_BG.srv.GetAddressOf());

	pContext->Draw(3,0);
}

inline int Round(int x) {
	return x;
}

inline int Round(float x) {
	return int(round(x));
}

void DX11Renderer::DrawMesh(MeshRef meshId, EShadingLayer layer)
{
	auto& meshData = m_MeshData[meshId];
	Mesh const& mesh = m_Scene->GetMesh(meshId);
	if (!meshData.vBuff)
	{
		PrepareMesh(mesh, meshData);
	}
	assert(meshData.iBuff);

	if (!m_Materials[mesh.material])
	{
		PrepareMaterial(mesh.material);
	}

	m_Materials[mesh.material]->Bind(m_Ctx, layer);



	PerInstancePSData PIPS;
	Material const& mat = m_Scene->Materials()[mesh.material];
	PIPS.colour = mat.colour;
	PIPS.ambdiffspec = {1, mat.diffuseness, mat.specularity};
	PIPS.specularExp = mat.specularExp;
	PIPS.useNormalMap = mat.normal->IsValid();
	WriteCBuffer(m_PSPerInstanceBuff, PIPS);

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

void DX11Renderer::PrepareMesh(Mesh const& mesh, DX11Mesh& meshData)
{
	printf("Creating buffers for mesh %s ", mesh.name.c_str());
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
	std::unique_ptr<DX11Material>& result = m_Materials[mid];
	Material const&					  mat = m_Scene->Materials()[mid];
	if (mat.albedo->IsValid())
	{
		auto  texResource = PrepareTexture(*mat.albedo, true);
		auto texMat = std::make_unique<DX11TexturedMaterial>(&m_MatTypes[MAT_TEX], texResource);
		if (mat.normal->IsValid())
		{
			texMat->m_Normal = PrepareTexture(*mat.normal);
		}
		if (mat.emissive->IsValid())
		{
			texMat->m_Emissive = PrepareTexture(*mat.emissive);
		}
		else
		{
			texMat->m_Emissive = m_EmptyTexture;
		}
		result = std::move(texMat);
	}
	else
	{
		result = std::make_unique<DX11Material>(&m_MatTypes[MAT_PLAIN]);
	}
}

void DX11Renderer::Resize(u32 width, u32 height, u32* canvas)
{
	m_Width = width;
	m_Height = height;
	m_Dirty = true;
	m_Scale = float(min(width, height));
}

ImageDX11::Ref DX11Renderer::PrepareTexture(Texture const& tex, bool sRGB)
{
	return ImageDX11::Create(pDevice, tex.width, tex.height, tex.GetData(), sRGB);
}

struct ShaderVariant
{
	ShaderVariant(String const& name, Vector<D3D_SHADER_MACRO>&& defines)
		: m_Name(name), m_Defines(std::move(defines)) {}
	String m_Name;
	Vector<D3D_SHADER_MACRO> m_Defines;
	ComPtr<ID3DBlob> m_Blob;
	bool m_Dirty = false;
};

int GetCompiledShaderVariants(char const* name, ShaderVariant* variants, u32 numVars, char const* shaderType)
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
		if (var.m_Dirty || !fs::exists(cso) || fs::last_write_time(cso) < lastWrite)
		{
			printf("Compiling shader in file %s\n", src.string().c_str());
			ID3DBlob* output;
			HRESULT hr = D3DCompileFromFile(src.wstring().c_str(), Addr(var.m_Defines), D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderType, flags, 0, &output, &errBlob);
			if (SUCCEEDED(hr))
			{
				var.m_Blob = output;
				printf("Saving to file %s\n", cso.string().c_str());
				D3DWriteBlobToFile(var.m_Blob.Get(), cso.wstring().c_str(), true);
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
			D3DReadFileToBlob(cso.c_str(), &var.m_Blob);
		}
	}
	return numVars;
}

void DX11Renderer::CreateMatType(u32 index, char const* vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize)
{
	ShaderVariant variants[] = {
		{ "base", {
			D3D_SHADER_MACRO{ "BASE_LAYER", "1" },
			{nullptr, nullptr}
		}},
		{ "dirlight", {
			D3D_SHADER_MACRO{ "DIR_LIGHT", "1" },
			{nullptr, nullptr}
		}},
		{ "pointlight", {
			D3D_SHADER_MACRO{ "POINT_LIGHT", "1" },
			{nullptr, nullptr}
		}},
		{ "spotlight", {
			D3D_SHADER_MACRO{ "SPOTLIGHT", "1" },
			{nullptr, nullptr}
		}},
	};

	if (GetCompiledShaderVariants(vsName, variants, Size(variants), "vs_5_0") > 0)
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
	int count = GetCompiledShaderVariants(psName, variants, Size(variants), "ps_5_0");
	for (int i=0; i<count; ++i )
	{
		auto const& pixBlob = variants[i].m_Blob;
		HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader[i]));
	}

}

void DX11Renderer::CreateMatTypeUnshaded(u32 index, char const* vsName, char const* psName, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize)
{
	ShaderVariant variants[] = {
		{ "base", {
			//D3D_SHADER_MACRO{ "AMBIENT", "1" },
			{nullptr, nullptr}
		}},
	};

	if (GetCompiledShaderVariants(vsName, variants, Size(variants), "vs_5_0") > 0)
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
	int count = GetCompiledShaderVariants(psName, variants, Size(variants), "ps_5_0");
	for (int i=0; i<count; ++i )
	{
		auto const& pixBlob = variants[i].m_Blob;
		HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader[i]));
	}

}

