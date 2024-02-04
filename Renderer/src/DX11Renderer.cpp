#include "DX11Renderer.h"
#include "Scene.h"
#include <functional>
#include <algorithm>
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

	col directionalCol;
	float _pad2;
	vec3 directionalDir;
	float _pad3;
};

__declspec(align(16))
struct PFPSPointLight : PerFramePSData
{
	vec3 pointLightPos;
	float pointLightRad;
	vec3 pointLightCol;
	float pointLightFalloff;

	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
};

__declspec(align(16))
struct PerFrameVertexData
{
	mat4 screen2World;
	mat4 world2Light;
	vec3 cameraPos;
};

struct PerInstanceVSData
{
	mat4 fullTransform;
	mat4 model2ShadeSpace;
	mat4 model2ShadeDual;
};

template <typename T>
void DX11Renderer::CreateConstantBuffer(ComPtr<ID3D11Buffer>& buffer, T initialData)
{
	D3D11_BUFFER_DESC cbDesc;
	Zero(cbDesc);
	cbDesc.ByteWidth = Sizeof(initialData);
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

struct ShadowMapCBuff
{
};

void DX11Renderer::PrepareShadowMaps()
{
	D3D11_TEXTURE2D_DESC shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	shadowMapDesc.Width = 1024;
	shadowMapDesc.Height = 1024;

	HR_ERR_CHECK(pDevice->CreateTexture2D(&shadowMapDesc, nullptr, &m_ShadowMap));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	HR_ERR_CHECK(pDevice->CreateDepthStencilView(m_ShadowMap.Get(), &depthStencilViewDesc, &m_ShadowDepthView));
	HR_ERR_CHECK(pDevice->CreateShaderResourceView(m_ShadowMap.Get(), &shaderResourceViewDesc, &m_ShadowSRV));

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
	pContext->OMSetRenderTargets(0, nullptr, m_ShadowDepthView.Get());
	pContext->ClearDepthStencilView(m_ShadowDepthView.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);

	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = 1024,
		.Height = 1024,
		.MinDepth = 0,
		.MaxDepth = 1,
	};
	pContext->RSSetViewports(1, &viewport);

	float const FAR_PLANE = 1000;
	float const NEAR_PLANE = .1f;
	m_Projection =
	{ 
		{ 2, 0, 0, 0 },
		{0,	2, 0,0},
		{ 0, 0, (FAR_PLANE) / (FAR_PLANE - NEAR_PLANE), -FAR_PLANE * NEAR_PLANE  / (FAR_PLANE - NEAR_PLANE)},
		{0,			0,		1,0},
	};
	m_Projection = glm::transpose(m_Projection);
	auto w2ltrans = m_Scene->m_Objects[0]->GetRoot()->GetWorldTransform();
	w2ltrans.SetScale(vec3(1));
	mat4 light2World = w2ltrans;
	mat4 projWorld = m_Projection * inverse(light2World);
	m_World2LightProj = projWorld; 

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
		DrawMesh(mi.mesh, true);
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

void DX11Renderer::Setup()
{
	//Setup matrix constant buffer
	CreateConstantBuffer<PerInstanceVSData>(m_VSPerInstanceBuff);
	CreateConstantBuffer<PerFrameVertexData>(m_VSPerFrameBuff);

	CreateConstantBuffer<PerInstancePSData>(m_PSPerInstanceBuff);
	CreateConstantBuffer<PFPSPointLight>(m_PSPerFrameBuff);


	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		//{ "Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//		{ "Colour", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(Vert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uvs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	m_MatTypes.resize(MAT_COUNT);
	CreateMatType(MAT_PLAIN, L"PlainVertexShader.cso",L"PlainPixelShader.cso",ied,Size(ied));
	CreateMatType(MAT_TEX, L"TexVertexShader.cso",L"TexPixelShader.cso",ied,Size(ied));

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
	
	CreateMatType(MAT_BG, L"BGVertexShader.cso", L"BGPixelShader.cso", ied, Size(ied));
	m_BG = CubemapDX11::FoldUp(pDevice, tex);
}


void DX11Renderer::Render(const Scene& scene)
{
	m_Scene = &scene;
	m_MeshData.resize(scene.GetAssetManager()->GetMeshes().size());
	m_Materials.resize(scene.m_Materials.size());

	RenderShadowMap();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	pContext->OMSetRenderTargets(1, m_MainRenderTarget.GetAddressOf(), m_MainDepthStencil.Get());
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	pContext->ClearRenderTargetView(m_MainRenderTarget.Get(), clear_color_with_alpha);
	pContext->ClearDepthStencilView(m_MainDepthStencil.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);

	m_Ctx.pixelSRVs.push_back(m_ShadowSRV.Get());

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

	PFPSPointLight perFrameData;
	perFrameData.ambient = vec3(scene.m_AmbientLight);
	perFrameData.directionalCol = vec3(scene.m_DirLights[0].intensity);
	perFrameData.directionalDir = scene.m_DirLights[0].dir;
	perFrameData.pointLightPos = scene.m_PointLights[0].pos;
	perFrameData.pointLightCol = scene.m_PointLights[0].intensity;
	perFrameData.pointLightRad = scene.m_PointLights[0].radius;
	perFrameData.pointLightFalloff = scene.m_PointLights[0].falloff;

	perFrameData.spotLightCol = scene.m_SpotLights[0].colour;
	perFrameData.spotLightDir = scene.m_SpotLights[0].dir;
	perFrameData.spotLightPos = scene.m_SpotLights[0].pos;
	perFrameData.spotLightTan = scene.m_SpotLights[0].tangle;
	perFrameData.spotLightFalloff = scene.m_SpotLights[0].falloff;

	WriteCBuffer(m_PSPerFrameBuff, perFrameData);

	PerFrameVertexData PFVD;
	PFVD.cameraPos = m_Camera->position;
	PFVD.screen2World = inverse(projWorld);
	PFVD.world2Light = m_World2LightProj;
	WriteCBuffer(m_VSPerFrameBuff, PFVD);

	for (auto const& mi : scene.m_MeshInstances)
	{
		if (mi.mesh == -1)
		{
			printf("Mesh instance with no mesh");
			continue;
		}
		Transform const& trans = mi.trans;
		Mesh const&		 mesh = scene.GetMesh(mi.mesh);

		PerInstanceVSData PIVS;
		PIVS.model2ShadeSpace = mat4(trans);
		PIVS.model2ShadeDual = transpose(inverse(PIVS.model2ShadeSpace));
		PIVS.fullTransform = projWorld * PIVS.model2ShadeSpace;

		WriteCBuffer(m_VSPerInstanceBuff,PIVS);

		ID3D11Buffer* vbuffs[] = { m_VSPerInstanceBuff.Get(), m_VSPerFrameBuff.Get() };
		pContext->VSSetConstantBuffers(0, Size(vbuffs), vbuffs);
		DrawMesh(mi.mesh);
	}
	m_Ctx.pixelSRVs.pop_back();
	DrawBG();
}


void DX11Renderer::DrawBG()
{
	const UINT stride = sizeof(BGVert);
	const UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, m_BGVBuff.GetAddressOf(), &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_MatTypes[MAT_BG].Bind(m_Ctx);
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

void DX11Renderer::DrawMesh(MeshRef meshId, bool shadow)
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

	m_Materials[mesh.material]->Bind(m_Ctx);

	if (shadow)
	{
		pContext->PSSetShader(nullptr, nullptr, 0);
	}


	PerInstancePSData PIPS;
	Material const& mat = m_Scene->m_Materials[mesh.material];
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
	Material const&					  mat = m_Scene->m_Materials[mid];
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

void DX11Renderer::CreateMatType(u32 index, LPCWSTR vertFilePath, LPCWSTR pixFilePath, const D3D11_INPUT_ELEMENT_DESC* ied, u32 iedsize)
{
	ComPtr<ID3DBlob> vertBlob;
	HR_ERR_CHECK(D3DReadFileToBlob(vertFilePath, &vertBlob));
	HR_ERR_CHECK(pDevice->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_VertexShader));

	ComPtr<ID3DBlob> pixBlob;
	HR_ERR_CHECK(D3DReadFileToBlob(pixFilePath, &pixBlob));
	HR_ERR_CHECK(pDevice->CreatePixelShader(pixBlob->GetBufferPointer(), pixBlob->GetBufferSize(), nullptr, &m_MatTypes[index].m_PixelShader));

	HR_ERR_CHECK(pDevice->CreateInputLayout(ied, iedsize, vertBlob->GetBufferPointer(), vertBlob->GetBufferSize(), &m_MatTypes[index].m_InputLayout));
}

