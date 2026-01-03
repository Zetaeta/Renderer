#include "render/dx11/DX11Renderer.h"
#include "scene/Scene.h"
#include <functional>
#include <algorithm>
#include <format>
#include <filesystem>
#include <thread>
#include <chrono>
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
#include "../../scene/StaticMeshComponent.h"
#include "render/MaterialManager.h"
#include "render/RendererScene.h"
#include "render/shaders/ShaderDeclarations.h"
#include "render/RenderController.h"
#include "backends/imgui_impl_dx11.h"
#include "common/ImguiThreading.h"
#include "DX11Swapchain.h"
#include "platform/windows/Window.h"
#include "backends/imgui_impl_win32.h"


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
}

DECLARE_CLASS_TYPEINFO(PerInstancePSData);

 DX11Renderer::DX11Renderer(Scene* scene, UserCamera* camera, u32 width, u32 height)
	: DX11Device(), m_Height(height), m_Width(width), m_Camera(camera), m_PixelWidth(width), pDevice(mDevice.Get()), pContext(mDeviceCtx.Get()), m_Scene(scene), mCBPool(pDevice, this)
{
	mCtx = &m_Ctx;
	CBPool = &mCBPool;
	Device = this;
	myDevice = this;
	m_Ctx.pDevice = pDevice;
	m_Ctx.pContext = pContext;
	m_Ctx.m_Renderer = this;
	Setup();
	mViewport = nullptr; //MakeOwning<Viewport>(this, scene, camera);

	D3D11_QUERY_DESC disjointDesc {};
	disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	for (int i = 0; i < DX11Timer::BufferSize; ++i)
	{
		pDevice->CreateQuery(&disjointDesc, &DisjointQueries[i]);
	}
	mFrameTimer = CreateTimer(L"Frame");
	pContext->QueryInterface<ID3DUserDefinedAnnotation>(&mUserAnnotation);

	GRenderController.AddRenderBackend(this);
}

DX11Renderer::~DX11Renderer()
{
	if (mRenderingImgui)
	{
		ThreadImgui::ImguiThreadInterface::Shutdown();
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	mSwapChains.clear();
	GRenderController.RemoveRenderBackend(this);
	RendererScene::OnShutdownDevice(this);
	IRenderDevice::Teardown();
	ResourceMgr.Teardown();
	MatMgr.Release();
	mTextures.clear();
	mRCtx = {};
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
	static const char* layers[] = { "base",
		"dirlight",
		"pointlight",
		"spotlight",
	};
	for (int i=0; i<Denum(EShadingLayer::ForwardRenderCount); ++i)
	{
		ImGui::Checkbox(layers[i], &m_Ctx.mRCtx->Settings.LayersEnabled[i]);
	}
	if (ImGui::Button("Full recompile shaders"))
	{
		ShaderMgr.RecompileAll(true);
	}

	ImGui::DragInt("Debug mode", reinterpret_cast<int*>(&mRCtx->Settings.ShadingDebugMode), 0.5, 0, Denum(EShadingDebugMode::COUNT));
	ImGui::DragFloat("Debug greyscale exponent", &mRCtx->Settings.DebugGrayscaleExp, 0.1f, 0, 10);
	ImGui::DragInt2("Viewer size", &mViewerSize.x, 1.f, 0, 2500);
	mRCtx->DrawControls();


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


	ImGui::End();
}

void DX11Renderer::Setup()
{
	//Setup matrix constant buffer
	m_VSPerInstanceBuff = DX11ConstantBuffer::Create<PerInstanceVSData>(&m_Ctx);
	m_VSPerFrameBuff = DX11ConstantBuffer::CreateWithLayout<PerFrameVertexData>(&m_Ctx);

	m_PSPerInstanceBuff = DX11ConstantBuffer::Create<PerInstancePSData>(&m_Ctx);

	m_PSPerFrameBuff = DX11ConstantBuffer(&m_Ctx, MaxSize<PFPSPointLight, PFPSSpotLight, PerFramePSData>());


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
void DX11Renderer::Render(const Scene& scene)
{
	ProcessTextureCreations();
	ID3D11SamplerState* samplers[] = { m_Sampler.Get(), m_ShadowSampler.Get() };

	m_Scene = const_cast<Scene*>(&scene);

	ImVec4 clear_color = ImVec4();

	//m_Scale = float(std::min(m_Width, m_Height));
	//m_Camera->SetViewExtent(.5f * m_Width / m_Scale, .5f * m_Height / m_Scale );

	if (mViewport)
	{
	if (!mViewport->mRScene->IsInitialized())
	{
		mViewport->mRScene = RendererScene::Get(scene, this);
	}

	}


	//m_Ctx.mRCtx->RenderFrame();

	SetDepthStencilMode(EDepthMode::Disabled);
	if (m_ViewerTex != nullptr)
	{
		DrawTexture(m_ViewerTex);
	}
	//pContext->OMSetRenderTargets(0, nullptr, nullptr);

	//pContext->PSSetShader(nullptr, nullptr, 0);
	//pContext->VSSetShader(nullptr, nullptr, 0);

	mRCtx->TextureManager.UnBind(this);
//	mViewport->mRScene->EndFrame();
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
		MatMgr.MatTypes()[MAT_2D].Bind(*mRCtx, EShadingLayer::BASE, E_MT_OPAQUE);
	}
	else
	{
		m_PS2DCBuff.WriteData(ivec2(m_Width, m_Height));
		MatMgr.MatTypes()[MAT_2D_UINT].Bind(*mRCtx, EShadingLayer::BASE, E_MT_OPAQUE);
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


void DX11Renderer::DrawMesh(IDeviceMesh const* mesh)
{
	pContext->IASetPrimitiveTopology(GetD3D11Topology(mesh->Topology));
	if (mInputLayoutDirty)
	{
		UpdateInputLayout();
	}

	ID3D11SamplerState* samplers[] = { m_Sampler.Get(), m_ShadowSampler.Get() };
	pContext->PSSetSamplers(0, 2, samplers );
	ZE_REQUIRE(mCurrVertexLayout);
	if (mesh->MeshType == EDeviceMeshType::DIRECT)
	{
		DX11DirectMesh const* mesh11 = static_cast<DX11DirectMesh const*>(mesh);
		const UINT		stride = mCurrVertexLayout->GetSize();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, mesh11->vBuff.GetAddressOf(), &stride, &offset);
		pContext->Draw(mesh11->VertexCount, 0);
	}
	else if (mesh->MeshType == EDeviceMeshType::INDEXED)
	{
		DX11IndexedMesh const* mesh11 = static_cast<DX11IndexedMesh const*>(mesh);
		const UINT		stride = mCurrVertexLayout->GetSize();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, mesh11->vBuff.GetAddressOf(), &stride, &offset);
		pContext->IASetIndexBuffer(mesh11->iBuff.Get(), DXGI_FORMAT_R16_UINT, offset);
		pContext->DrawIndexed(mesh11->IndexCount, 0, 0);
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

rnd::MappedResource DX11Renderer::Readback(DeviceResourceRef resource, u32 subresource, _Out_opt_ RefPtr<GPUSyncPoint>* completionSyncPoint)
{
	struct DX11DummySyncPoint : public GPUSyncPoint
	{
		void OnFullyReleased() override
		{
			delete this;
		}
		bool Wait(u32 forMS) override
		{
			return true;
		}
	};

	if (completionSyncPoint)
	{
		*completionSyncPoint = new DX11DummySyncPoint;
	}

	if (resource->GetResourceType() == EResourceType::Texture2D)
	{
		auto* tex = static_cast<DX11Texture*>(resource.get());
		MappedResource result = tex->Map(subresource, ECpuAccessFlags::Read);
		result.Release = [resource, subresource]
		{
			static_cast<DX11Texture*>(resource.get())->Unmap(subresource);
		};
		return result;
	}
	else //if (resource->GetResourceType() == EResourceType::Buffer)
	{
		ZE_ASSERT(false);
		return {};
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

void DX11Renderer::SetBackbuffer(DX11Texture::Ref backBufferTex, u32 width, u32 height)
{
	m_MainRenderTarget = static_pointer_cast<DX11RenderTarget>(backBufferTex->GetRT().RT);

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
	return;
	mViewport->Reset();
//	pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

	if (width > 0 && height > 0)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
//		pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		DeviceTextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.Flags = TF_RenderTarget | TF_SRV;// | TF_SRGB;
		SetBackbuffer(std::make_shared<rnd::dx11::DX11Texture>(m_Ctx,desc, backBuffer.Get()), width, height);
	}
}

void DX11Renderer::ImGuiInit(wnd::Window* mainWindow)
{
	ImGui_ImplWin32_Init(mainWindow->GetHwnd());
	ImGui_ImplDX11_Init(m_Ctx.pDevice, m_Ctx.pContext);
	ImGui_ImplDX11_CreateDeviceObjects();
	ThreadImgui::ImguiThreadInterface::Init();
	mRenderingImgui = true;
}

void DX11Renderer::PreImgui()
{
	if (!mSwapChains.empty())
	{
		auto rt = mSwapChains[0]->GetBackbuffer()->GetRT()->GetData<ID3D11RenderTargetView*>();
		pContext->OMSetRenderTargets(1, &rt, nullptr);
	}
}

void DX11Renderer::ImguiBeginFrame()
{
	if (mRenderingImgui)
	{
		ImGui_ImplDX11_NewFrame();
	}
//	ThreadImgui::BeginFrame();
}

void DX11Renderer::ImguiEndFrame()
{
	if (!mRenderingImgui)
	{
		return;
	}
	//ImGui::End();
	//ImGui::Render();
//	ThreadImgui::EndFrame();
	PreImgui();
	ThreadImgui::ImDrawDataWrapper drawData = ThreadImgui::GetDrawData_RenderThread();
	ImGui_ImplDX11_RenderDrawData(&drawData);
	ThreadImgui::RenderPlatformWindows();
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
	auto result = GetRenderTexture(&tex);
	if (!result)
	{
		return m_EmptyTexture;
	}
	return result;
}

template<typename T>
bool GetQueryWithRetry(ID3D11DeviceContext* context, ID3D11Query* Query, T& outVal, UINT flags = 0)
{
	HRESULT hres = S_FALSE;
	for (;;)
	{
		hres = context->GetData(Query, &outVal, sizeof(T), 0);
		HR_ERR_CHECK(hres);
		if (hres == S_OK)
		{
			return true;
		}
		else
		{
			using namespace std::chrono;
			std::this_thread::sleep_for(5ms);
		}
	}
	return false;
}

void DX11Renderer::ProcessTimers()
{
	if (mFrameNum <= 3)
	{
		return;
	}
	u32 frameIdx = mFrameNum % DX11Timer::BufferSize;
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
//	ZE_ENSURE(hr==S_OK);
	GetQueryWithRetry(pContext, DisjointQueries[frameIdx].Get(), disjointData);
	//if (disjointData.Disjoint)
	//{
	//	for (u32 i = 0; i < mTimerPool.Size(); ++i)
	//	{
	//		mTimerPool[i].GPUTimeMs = -1.f;
	//	}
	//	return;
	//}
	for (u32 i = 0; i < mTimerPool.Size(); ++i)
	{
		DX11Timer& timer = mTimerPool[i];
//			ZE_ASSERT(timer.StartCount == timer.EndCount);
		ZE_ASSERT((timer.WasUsed[frameIdx] % 2) == 0);
		bool bGetResult = timer.WasUsed[frameIdx] > 0;

		if (bGetResult)
		{
			u64 start, end;
			GetQueryWithRetry(pContext, timer.Queries[frameIdx].Start.Get(), start);
			GetQueryWithRetry(pContext, timer.Queries[frameIdx].End.Get(), end);
			if (!disjointData.Disjoint)
			{
				timer.GPUTimeMs = NumCast<float>((double(end - start) * 1000.) / disjointData.Frequency);
			}
			else
			{
				timer.GPUTimeMs = -1;
			}
		}

		if (mTimerPool.IsInUse(i))
		{
			if (timer.GetRefCount() == 0)
			{
				mTimerPool.Release(i);
			}
		}

		mTimerPool[i].WasUsed[frameIdx] = 0;
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
	auto* dx11Timer = static_cast<DX11Timer*>(timer);
	pContext->End(dx11Timer->Queries[currFrameIdx].Start.Get());
//	++dx11Timer->StartCount;
	dx11Timer->WasUsed[currFrameIdx] = 1;
	if (mUserAnnotation)
	{
		mUserAnnotation->BeginEvent(timer->Name.c_str());
	}
}

void DX11Renderer::StopTimer(GPUTimer* timer)
{
	u32 currFrameIdx = NumCast<u32>(mFrameNum % DX11Timer::BufferSize);
	auto* dx11Timer = static_cast<DX11Timer*>(timer);
	pContext->End(dx11Timer->Queries[currFrameIdx].End.Get());
	ZE_ASSERT(dx11Timer->WasUsed[currFrameIdx] == 1);
	dx11Timer->WasUsed[currFrameIdx] = 2;
		//	++dx11Timer->EndCount;
	if (mUserAnnotation)
	{
		mUserAnnotation->EndEvent();
	}
}
#endif

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

RenderMaterial* DX11Renderer::GetDefaultMaterial(int matType)
{
	auto& materialType = MatMgr.MatTypes()[matType];
	if (!IsValid(materialType.m_Default))
	{
		materialType.m_Default = std::make_unique<RenderMaterial>(&materialType, E_MT_OPAQUE, StandardMatProperties{});
		
	}
	return materialType.m_Default.get();
}

IDeviceTexture::Ref DX11Renderer::CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData)
{
	if (initialData.Tex && initialData.Format == ECubemapDataFormat::FoldUp)
	{
		return std::make_shared<DX11Cubemap>(DX11Cubemap::FoldUp(m_Ctx, initialData.Tex));
	}
	return std::make_shared<DX11Cubemap>(m_Ctx, desc);
}

IDeviceTexture::Ref DX11Renderer::CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData)
{
//	return DX11Cubemap::FoldUp(m_Ctx, )
//	return DX11Texture::Create(&m_Ctx, desc.width, desc.height, reinterpret_cast<u32 const*>(initialData), TF_DEPTH);
	return std::make_shared<DX11Texture>(m_Ctx, desc, initialData);
}

void DX11Renderer::ExecuteCommand(std::function<void(IRenderDeviceCtx&)>&& command, char const* name)
{
	command(*this);
}

void DX11Renderer::BeginFrame()
{
	ImguiBeginFrame();
	++mFrameNum;
	ProcessTimers();
	for (auto& swapChain : mSwapChains)
	{
		if (swapChain->IsResizeRequested())
		{
			swapChain->Resize();
		}
	}

	StartFrameTimer();

	ProcessTextureCreations();

	ImVec4 clear_color = ImVec4();

	//m_Scale = float(std::min(m_Width, m_Height));
	//m_Camera->SetViewExtent(.5f * m_Width / m_Scale, .5f * m_Height / m_Scale );

	//if (!mViewport->mRScene->IsInitialized())
	//{
	//	mViewport->mRScene = RendererScene::Get(scene, this);
	//}
}

void DX11Renderer::EndFrame()
{
	EndFrameTimer();
	ImguiEndFrame();

	for (auto& swapChain : mSwapChains)
	{
		swapChain->Present();
	}
//	mCBPool.OnEndFrame();
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
#define X(shader) ClearResourceBindings(EShaderType::shader);
	FOR_EACH_SHADER_FREQ(X)
#undef X
}

void DX11Renderer::ClearResourceBindings(EShaderType ShaderStage)
{
	ID3D11ShaderResourceView* const* dummyViews = reinterpret_cast<ID3D11ShaderResourceView* const*>(ZerosArray);
	switch (ShaderStage)
	{
	case EShaderType::Vertex:
		pContext->VSSetShaderResources(0, mMaxShaderResources[EShaderType::Vertex], dummyViews);
		break;
	case rnd::EShaderType::Pixel:
		pContext->PSSetShaderResources(0, mMaxShaderResources[EShaderType::Pixel], dummyViews);
		pContext->OMSetRenderTargets(0, nullptr, nullptr);
		break;
	case rnd::EShaderType::Compute:
		pContext->CSSetShaderResources(0, mMaxShaderResources[EShaderType::Compute], dummyViews);
		if (mMaxUAVs[EShaderType::Compute] > 0)
		{
			pContext->CSSetUnorderedAccessViews(0, mMaxUAVs[EShaderType::Compute], reinterpret_cast<ID3D11UnorderedAccessView* const*>(ZerosArray), nullptr);
		}
		break;
	default:
		break;
	}
}

void DX11Renderer::SetPixelShader(PixelShader const* shader)
{
	DX11PixelShader* dx11Shader = shader ? static_cast<DX11PixelShader*>(shader->GetDeviceShader()) : nullptr;
	pContext->PSSetShader(dx11Shader ? dx11Shader->GetShader() : nullptr, nullptr, 0);
}

void DX11Renderer::SetVertexShader(VertexShader const* shader)
{
	//pContext->IASetInputLayout(nullptr);
	mCurrVertexShader = shader;
	DX11VertexShader* dx11Shader = shader ? static_cast<DX11VertexShader*>(shader->GetDeviceShader()) : nullptr;
	pContext->VSSetShader(dx11Shader ? dx11Shader->GetShader() : nullptr, nullptr, 0);
	mInputLayoutDirty = true;
}

void DX11Renderer::SetComputeShader(ComputeShader const* shader)
{
	DX11ComputeShader* dx11Shader = shader ? static_cast<DX11ComputeShader*>(shader->GetDeviceShader()) : nullptr;
	pContext->CSSetShader(dx11Shader ? dx11Shader->GetShader() : nullptr, nullptr, 0);
}

void DX11Renderer::SetVertexLayout(VertAttDescHandle attDescHandle)
{
	if (attDescHandle == mCurrVertexLayoutHdl)
	{
		return;
	}
	mCurrVertexLayoutHdl = attDescHandle;
	mInputLayoutDirty = true;
	if (mCurrVertexLayoutHdl >= 0)
		mCurrVertexLayout = &VertexAttributeDesc::GetRegistry().Get(attDescHandle);
}

void DX11Renderer::UpdateInputLayout()
{
	mInputLayoutDirty = false;
	if (!ZE_ENSURE(mCurrVertexShader && mCurrVertexLayoutHdl >= 0))
	{
		pContext->IASetInputLayout(nullptr);
		return;
	}
	ID3D11InputLayout* inputLayout = GetOrCreateInputLayout(mCurrVertexLayoutHdl, mCurrVertexShader->GetInputSignature());
	pContext->IASetInputLayout(inputLayout);
}

void DX11Renderer::SetRTAndDS(IRenderTarget::Ref rt, IDepthStencil::Ref ds, int RTArrayIdx, int DSArrayIdx)
{
	auto*					dx11DS = static_cast<DX11DepthStencil*>(ds.DS.get());
	ID3D11DepthStencilView* dsv= nullptr;
	D3D11_VIEWPORT vp {};
	vp.MaxDepth = 1;
	D3D11_RECT scissor{};
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
		vp.Width = float(ds.DS->Desc.Width);
		vp.Height = float(ds.DS->Desc.Height);
		scissor = CD3D11_RECT(0, 0, ds.DS->Desc.Width, ds.DS->Desc.Height);
	}

	ID3D11RenderTargetView* rtv= nullptr;
	auto*					dx11RT = static_cast<DX11RenderTarget*>(rt.RT.get());
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
		vp.Width = float(rt->Desc.Width);
		vp.Height = float(rt->Desc.Height);
		scissor = CD3D11_RECT(0, 0, rt->Desc.Width, rt.RT->Desc.Height);
	}
//	scissor = CD3D11_RECT(0, 0, vp.Width, vp.Height);
	
	pContext->OMSetRenderTargets(1, &rtv, dsv);
	pContext->RSSetViewports(1, &vp);
	pContext->RSSetScissorRects(1, &scissor);
}

void DX11Renderer::SetRTAndDS(Span<IRenderTarget::Ref> rts, IDepthStencil::Ref ds)
{
	std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> renderTargets;
	ZE_ASSERT(rts.size() <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
	D3D11_VIEWPORT vp {};
	vp.MaxDepth = 1;
	D3D11_RECT scissor{};
	for (u32 i=0; i<rts.size(); ++i)
	{
		if (rts[i] != nullptr)
		{
			renderTargets[i] = static_cast<DX11RenderTarget*>(rts[i].RT.get())->GetRTV();
			vp.Width = float(rts[i].RT->Desc.Width);
			vp.Height = float(rts[i].RT->Desc.Height);
			scissor = CD3D11_RECT(0, 0, rts[i]->Desc.Width, rts[i]->Desc.Height);
		}
	}

	auto* dx11DS = static_cast<DX11DepthStencil*>(ds.get());
	ID3D11DepthStencilView* dsv = dx11DS ? dx11DS->GetDSV() : nullptr;
	if (ds)
	{
		vp.Width = float(ds->Desc.Width);
		vp.Height = float(ds->Desc.Height);
		scissor = CD3D11_RECT(0, 0, ds->Desc.Width, ds->Desc.Height);
	}

	pContext->OMSetRenderTargets(NumCast<u32>(rts.size()), &renderTargets[0], dsv);
	pContext->RSSetViewports(1, &vp);
	pContext->RSSetScissorRects(1, &scissor);
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

