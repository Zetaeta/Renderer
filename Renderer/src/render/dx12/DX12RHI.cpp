#include "DX12Window.h"

#include "core/WinUtils.h"
#include <d3d12sdklayers.h>
#include "dxgi1_6.h"
#include <d3dx12.h>
#include "core/Utils.h"
#include "DX12DescriptorHeap.h"
#include "DX12Test.h"
#include "render/DeviceShader.h"
#include "DX12Allocator.h"
#include "thread/ThreadUtils.h"
#include "DX12Texture.h"
#include "DX12DescriptorAllocator.h"
#include "DX12ShaderCompiler.h"
#include "render/dxcommon/DXGIUtils.h"

#pragma comment(lib, "d3d12.lib")

namespace rnd
{
namespace dx12
{

static DX12RHI* sRHI = nullptr;

DX12RHI::DX12RHI(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers)
:Window(width, height, name), IRenderDevice(new DX12LegacyShaderCompiler), mNumBuffers(numBuffers)
{
	//WNDCLASSEX wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DX12 window", nullptr };
	//ZE_ASSERT(RegisterClassEx(&wc) != 0);
	//mHwnd = ::CreateWindowW(wc.lpszClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, width, height, nullptr, nullptr, wc.hInstance, nullptr);
	//ZE_ASSERT(mHwnd != 0);

	//ShowWindow(mHwnd, SW_SHOWDEFAULT);
	//::UpdateWindow(mHwnd);

	ZE_ASSERT(sRHI == nullptr);
	sRHI = this;

	CreateDeviceAndCmdQueue();
	DX12DescriptorHeap::GetDescriptorSizes(mDevice.Get());


	DX12SyncPointPool::Create(mDevice.Get());
	mSimpleAllocator = MakeOwning<DX12Allocator>(mDevice.Get());

	mFenceEvent = CreateEventW(nullptr, false, false, nullptr);
	mCBPool = DX12CBPool(*this);
	mUploadBuffer = MakeOwning<DX12UploadBuffer>(1024 * 1024 * 256);
	mUploader = DX12Uploader(mDevice.Get());

	mShaderResourceDescAlloc = MakeOwning<DX12SeparableDescriptorAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64 * 1024, false);
	mRTVAlloc = MakeOwning<DX12SeparableDescriptorAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64, false);
	mDSVAlloc = MakeOwning<DX12SeparableDescriptorAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64, false);
	mShaderResourceDescTables = MakeOwning<DX12DescriptorTableAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mSamplerDescTables = MakeOwning<DX12DescriptorTableAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	mTest = MakeOwning<DX12Test>(mDevice.Get());

	WaitForGPU();
}

DX12RHI::~DX12RHI()
{
	mTest = nullptr;
	mUploadBuffer = nullptr;
	mCBPool.ReleaseResources();
	DX12SyncPointPool::Destroy();
	mRTVAlloc = nullptr;
	mDSVAlloc = nullptr;
	mShaderResourceDescAlloc = nullptr;

	WaitFence(mCurrentFrame);
	for (auto& deferredReleases : mDeferredReleaseResources)
	{
		deferredReleases.clear();
	}
	
	sRHI = nullptr;
}

void DX12RHI::Tick()
{
	if (mClosed)
	{
		return;
	}
	if (mResizeHeight != 0)
	{
		ResizeSwapChain();
	}
	StartFrame();
	auto& cmd = mCmdList.CmdList;
	cmd->Reset(mCmdList.Allocators[mFrameIndex].Get(), mTest->mPSO.Get());
	mRecordingCommands = true;

	D3D12_VIEWPORT vp {};
	vp.Height = float(mHeight);
	vp.Width = float(mWidth);
	vp.TopLeftX = vp.TopLeftY = 0;
	vp.MaxDepth = 1;
	vp.MinDepth = 0;
	cmd->RSSetViewports(1, &vp);
	CD3DX12_RECT scissor(0, 0, mWidth, mHeight);
	cmd->RSSetScissorRects(1, &scissor);

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mRenderTargets[mBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd->ResourceBarrier(1, &barrier);
	}
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mBufferIndex, mRTVHeap.ElementSize);
	cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
	float const clearColor[4] = {0.5f, 0.5f, 0.5f, 1.f};
	cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	mTest->Render(*cmd.Get());
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmd->ResourceBarrier(1, &barrier);
	}
	mRecordingCommands = false;
	cmd->Close();
	ID3D12CommandList* const cmdList = cmd.Get();
	mQueues.Direct->ExecuteCommandLists(1, &cmdList);
	EndFrame();
}

u64 DX12RHI::GetCompletedFrame() const
{
	return mFrameFence->GetCompletedValue();
}

void DX12RHI::WaitFence(u64 value)
{
	
	if (mFrameFence->GetCompletedValue() < value)
	{
		HR_ERR_CHECK(mFrameFence->SetEventOnCompletion(value, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void DX12RHI::WaitAndReleaseFrame(u64 frame)
{
	ZE_ASSERT(frame < mCurrentFrame);
	WaitFence(frame);
	ProcessDeferredRelease(frame % mNumBuffers);
}


void DX12RHI::WaitAndReleaseFrameIdx(u32 frameIdx)
{
	
	WaitFence(mFenceValues[frameIdx]);
	ProcessDeferredRelease(frameIdx);
}

void DX12RHI::StartFrame()
{
	WaitAndReleaseFrameIdx(mFrameIndex);
	//if (mFenceValue > 2)
	//{

	//}
}

void DX12RHI::EndFrame()
{
	HR_ERR_CHECK(mSwapChain->Present(1, 0));
	++mCurrentFrame;
	mFenceValues[mFrameIndex] = mCurrentFrame;
//	mCurrentFrame = mFenceValues[mFrameIndex];

	HR_ERR_CHECK(mQueues.Direct->Signal(mFrameFence.Get(), mCurrentFrame));
	mFrameIndex = mCurrentFrame % mNumBuffers;
	mBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void DX12RHI::WaitForGPU()
{
	DX12SyncPoint syncPoint = DX12SyncPointPool::Get()->Claim();
	syncPoint.GPUSignal(mQueues.Direct.Get());
	syncPoint.Wait();
	//++mCurrentFenceValue;
	//HR_ERR_CHECK(mCmdQueue->Signal(mFrameFence.Get(), mCurrentFenceValue));
	//HR_ERR_CHECK(mFrameFence->SetEventOnCompletion(mCurrentFenceValue, mFenceEvent));
	//WaitForSingleObject(mFenceEvent, INFINITE);
	//mFenceValues[mFrameIndex] = mCurrentFenceValue;

}

void DX12RHI::DeferredRelease(ComPtr<ID3D12Pageable>&& resource)
{
	mDeferredReleaseResources[mFrameIndex].emplace_back(std::move(resource));
}

void DX12RHI::ProcessDeferredRelease(u32 frameIndex)
{
	mDeferredReleaseResources[frameIndex].clear();
}

void DX12RHI::Resize_WndProc(u32 resizeWidth, u32 resizeHeight)
{
	mResizeWidth = resizeWidth;
	mResizeHeight = resizeHeight;
}

void DX12RHI::OnDestroy_WndProc()
{
	wnd::Window::OnDestroy_WndProc();
	mClosed = true;
}

void DX12RHI::CreateDeviceAndCmdQueue()
{
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif


	ComPtr<IDXGIFactory6> factory;
	HR_ERR_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> adapter;

	for (u32 adapterIdx = 0; SUCCEEDED(factory->EnumAdapterByGpuPreference(adapterIdx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); ++adapterIdx)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice))))
		{
			break;
		}
	}
	ZE_ASSERT(mDevice);

	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(mDevice.As<ID3D12InfoQueue>(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}

	//D3D12_COMMAND_QUEUE_DESC queueDesc {};
	//queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//HR_ERR_CHECK(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue)));
	mQueues.Direct = DX12CommandQueue(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	mQueues.Copy = DX12CommandQueue(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COPY);
	mQueues.Compute = DX12CommandQueue(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = mNumBuffers;
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
//	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	ComPtr<IDXGISwapChain1> swapChain;
	HR_ERR_CHECK(factory->CreateSwapChainForHwnd(mQueues.Direct, mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain));
	HR_ERR_CHECK(swapChain.As<IDXGISwapChain3>(&mSwapChain));
	factory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER);

	// Create command list + allocators
	HR_ERR_CHECK(mDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCmdList.CmdList)));
	for (u32 i = 0; i < mNumBuffers; ++i)
	{
		HR_ERR_CHECK(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdList.Allocators[i])));
	}

	GetSwapChainBuffers();

	mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence));


//	HR_ERR_CHECK(m)
}

void DX12RHI::ResizeSwapChain()
{
	WaitForGPU();
	mRTVHeap = {};
	mWidth = mResizeWidth;
	mHeight = mResizeHeight;
	mResizeWidth = mResizeHeight = 0;
	for (int i = 0; i < mNumBuffers; ++i)
	{
		mRenderTargets[i] = nullptr;
	}

	mSwapChain->ResizeBuffers(mNumBuffers, mWidth, mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	GetSwapChainBuffers();
}

void DX12RHI::GetSwapChainBuffers()
{

	mBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	mRTVHeap = DX12DescriptorHeap(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mNumBuffers);

	// swapchain render targets
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{mRTVHeap->GetCPUDescriptorHandleForHeapStart()};
		for (u32 i = 0; i < mNumBuffers; ++i)
		{
			HR_ERR_CHECK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
			mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, mRTVHeap.ElementSize);
		}
	}
}

ComPtr<ID3D12Resource> DX12RHI::CreateVertexBuffer(VertexBufferData data, ID3D12GraphicsCommandList_* uploadCmdList)
{

	constexpr u32 VertexBufferAlignment = 4;
	u32 size = NumCast<u32>(data.VertSize * data.NumVerts);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_NONE, VertexBufferAlignment);
	auto finalLocation = GetAllocator(EResourceType::VertexBuffer, data.VertSize * data.NumVerts)->Allocate(desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);
	auto uploadLoc = mUploadBuffer->Reserve(size, VertexBufferAlignment);
	memcpy(uploadLoc.WriteAddress, data.Data, size);
	uploadCmdList->CopyBufferRegion(finalLocation.Get(), 0, mUploadBuffer->GetCurrentBuffer(), uploadLoc.Offset, size);
	return finalLocation;
}

DeviceMeshRef DX12RHI::CreateDirectMesh(EPrimitiveTopology topology, VertexBufferData data, BatchedUploadHandle uploadHandle)
{
	u32 size = NumCast<u32>(data.VertSize * data.NumVerts);
	auto uploadCtx = mUploader.StartUpload();
	auto* result = mMeshes.Acquire();
	result->mResource = CreateVertexBuffer(data, uploadCtx.CmdList.Get());
	result->VertexAttributes = data.VertAtts;
	result->Topology = topology;
	result->VertexCount = data.NumVerts;
	result->view.BufferLocation = result->mResource->GetGPUVirtualAddress();
	
	result->view.SizeInBytes = size;
	result->view.StrideInBytes = data.VertSize;
	uploadCtx.FinishUploadSynchronous();
//	mDevice->create
//	mUploader.
//	UpdateSubresources()
	return result;
}

RefPtr<IDeviceIndexedMesh> DX12RHI::CreateIndexedMesh(EPrimitiveTopology topology, VertexBufferData vertexBuffer, Span<u16> indexBuffer, BatchedUploadHandle uploadHandle)
{
	u32 vbSize = NumCast<u32>(vertexBuffer.VertSize * vertexBuffer.VertAtts);
	auto uploadCtx = mUploader.StartUpload();
	auto* result = mIndexedMeshes.Acquire();
	result->VertBuff = CreateVertexBuffer(vertexBuffer, uploadCtx.CmdList.Get());
	result->VertBuffView.BufferLocation = result->VertBuff->GetGPUVirtualAddress();
	
	result->VertBuffView.SizeInBytes = vbSize;
	result->VertBuffView.StrideInBytes = vertexBuffer.VertSize;

	constexpr u32 IndexBufferAlignment = 4;
	u32 ibSize = NumCast<u32>(indexBuffer.size() * sizeof(u16));
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(ibSize, D3D12_RESOURCE_FLAG_NONE, IndexBufferAlignment);
	auto ibResource = GetAllocator(EResourceType::VertexBuffer, ibSize)->Allocate(desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);
	auto uploadLoc = mUploadBuffer->Reserve(ibSize, IndexBufferAlignment);
	memcpy(uploadLoc.WriteAddress, indexBuffer.data(), indexBuffer.size());;
	uploadCtx.CmdList->CopyBufferRegion(ibResource.Get(), 0, mUploadBuffer->GetCurrentBuffer(), uploadLoc.Offset, ibSize);
	result->IndBuff = ibResource;
	result->IndBuffView.BufferLocation = ibResource->GetGPUVirtualAddress();
	result->IndBuffView.Format = DXGI_FORMAT_R16_UINT;
	result->IndBuffView.SizeInBytes = ibSize;

	uploadCtx.FinishUploadSynchronous();
	return result;
}


bool DX12RHI::ShouldDirectUpload(EResourceType resourceType, u64 size)
{
	return IsInRenderThread() && IsRecordingCommands();
}

IDeviceTexture::Ref DX12RHI::CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData /*= nullptr*/)
{
	auto result = std::make_shared<DX12Texture>(desc);
	if (!initialData)
	{
		return result;
	}

	u64 uploadSize = GetRequiredIntermediateSize(result->GetTextureHandle<ID3D12Resource*>(), 0, desc.NumMips); // TODO mips
	auto alloc = mUploadBuffer->Reserve(uploadSize, desc.SampleCount > 1 ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
																		: D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	D3D12_SUBRESOURCE_DATA srcData{};
	srcData.pData = initialData;
	srcData.RowPitch = desc.GetRowPitch();

	if (ShouldDirectUpload(desc.ResourceType))
	{
		UpdateSubresources<1>(mCmdList.CmdList.Get(), result->GetTextureHandle<ID3D12Resource*>(), mUploadBuffer->GetCurrentBuffer(), alloc.Offset,
			0u, desc.NumMips == 0 ? 1u : desc.NumMips, &srcData); // TODO mips
		result->UpdateCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
	}
	else // synchronous upload on copy queue for now
	{
		auto& uploadCtx = mUploader.StartUpload();
		UpdateSubresources<1>(uploadCtx.CmdList.Get(), result->GetTextureHandle<ID3D12Resource*>(), mUploadBuffer->GetCurrentBuffer(), alloc.Offset,
			0u, desc.NumMips == 0 ? 1u : desc.NumMips, &srcData); // TODO mips
		uploadCtx.FinishUploadSynchronous();
		result->UpdateCurrentState(D3D12_RESOURCE_STATE_COMMON); // decays to common after used on copy queue
	}

	return result;
}

DX12DescriptorAllocator* DX12RHI::GetDescriptorAllocator(EDescriptorType descType)
{
	switch (descType)
	{
	case EDescriptorType::SRV:
	case EDescriptorType::UAV:
	case EDescriptorType::CBV:
		return mShaderResourceDescAlloc.get();
	case EDescriptorType::RTV:
		return mRTVAlloc.get();
	case EDescriptorType::DSV:
		return mDSVAlloc.get();
	default:
		return nullptr;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RHI::AllocateDescriptor(EDescriptorType descType)
{
	return GetDescriptorAllocator(descType)->Allocate(descType);
}

void DX12RHI::FreeDescriptor(EDescriptorType descType, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	GetDescriptorAllocator(descType)->Free(descriptor);
}

OwningPtr<rnd::dx12::DX12RHI::LiveObjectReporter> DX12RHI::GetLiveObjectReporter()
{
	auto result = MakeOwning<LiveObjectReporter>();
	HR_ERR_CHECK(mDevice.As<ID3D12DebugDevice1>(&result->DebugDevice));
	return result;
}

static D3D12_SHADER_BYTECODE GetBytecode(Shader const* shader)
{
	IDeviceShader* ds = shader ? shader->GetDeviceShader() : nullptr;
	if (ds == nullptr)
	{
		return {};
	}
	auto& blob = static_cast<DX12Shader*>(ds)->Bytecode;
	return {blob->GetBufferPointer(), blob->GetBufferSize()};
}

static void ApplyBlendState(EBlendState state, D3D12_BLEND_DESC& outDesc)
{
	auto& rtb = outDesc.RenderTarget[0];
	outDesc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT{});
	if (state == (EBlendState::COL_OVERWRITE | EBlendState::ALPHA_OVERWRITE))
	{
		return;
	}

	rtb.BlendEnable = true;
	if (!!(state & EBlendState::COL_ADD))
	{
		rtb.SrcBlend = D3D12_BLEND_ONE;
		rtb.DestBlend = D3D12_BLEND_ONE;
	}
	else if (HasAllFlags(state, EBlendState::COL_BLEND_ALPHA | EBlendState::COL_OBSCURE_ALPHA))
	{
		rtb.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rtb.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	}
	else if (HasAnyFlags(state, EBlendState::COL_BLEND_ALPHA))
	{

		rtb.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rtb.DestBlend = D3D12_BLEND_ONE;
	}

	if (HasAnyFlags(state, EBlendState::ALPHA_ADD))
	{
		rtb.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtb.DestBlendAlpha = D3D12_BLEND_ONE;
	}
	else if (HasAnyFlags(state, EBlendState::ALPHA_OVERWRITE))
	{
		rtb.SrcBlendAlpha = D3D12_BLEND_ZERO;
		rtb.DestBlendAlpha = D3D12_BLEND_ONE;
	}
	else if (HasAnyFlags(state, EBlendState::ALPHA_MAX))
	{
		rtb.BlendOpAlpha = D3D12_BLEND_OP_MAX;
		rtb.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtb.DestBlendAlpha = D3D12_BLEND_ONE;
	}
}

static void ApplyDepthMode(EDepthMode mode, D3D12_DEPTH_STENCIL_DESC& outDesc)
{
	if (HasAnyFlags(mode, EDepthMode::NoWrite))
	{
		outDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	else
	{
		outDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	}

	mode &= ~EDepthMode::NoWrite;
	outDesc.DepthEnable = true;
	switch (mode)
	{
	case EDepthMode::Disabled:
		outDesc.DepthEnable = false;
		break;
	case EDepthMode::Less:
		outDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		break;
	case EDepthMode::LessEqual:
		outDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		break;
	case EDepthMode::Equal:
		outDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
		break;
	case EDepthMode::GreaterEqual:
		outDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		break;
	case EDepthMode::Greater:
		outDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
		break;
	default:
		ZE_ASSERT(false);
		break;
	}
}

static void ApplyStencilMode(EStencilMode mode, D3D12_DEPTH_STENCIL_DESC& outDesc)
{
	if (HasAllFlags(mode, EStencilMode::Overwrite))
	{
		outDesc.StencilEnable = true;
		outDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		outDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	}
	if (HasAllFlags(mode, EStencilMode::IgnoreDepth))
	{
		outDesc.StencilEnable = true;
		outDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		outDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		outDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
		outDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
	}
	if (HasAnyFlags(mode, EStencilMode::IgnoreDepth))
	{
		outDesc.BackFace = outDesc.FrontFace;
	}
}

ID3D12PipelineState* DX12RHI::GetPSO(GraphicsPSODesc const& PSODesc)
{
	auto& pso = mPSOs[PSODesc];
	if (pso == nullptr)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
		desc.pRootSignature = PSODesc.RootSig.Get();
		desc.VS = GetBytecode(PSODesc.Shaders[EShaderType::Vertex]);
		desc.PS = GetBytecode(PSODesc.Shaders[EShaderType::Pixel]);
		desc.GS = GetBytecode(PSODesc.Shaders[EShaderType::Geometry]);
		desc.DS = GetBytecode(PSODesc.Shaders[EShaderType::TessEval]);
		desc.HS = GetBytecode(PSODesc.Shaders[EShaderType::TessControl]);
		ApplyBlendState(PSODesc.BlendState, desc.BlendState);
		desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT{});
		ApplyDepthMode(PSODesc.DepthMode, desc.DepthStencilState);
		ApplyStencilMode(PSODesc.StencilMode, desc.DepthStencilState);
		
		SmallVector<D3D12_INPUT_ELEMENT_DESC, 8> inputElements;
		{
			VertexAttributeDesc const& vertAtts = VertexAttributeDesc::GetRegistry().Get(PSODesc.VertexLayoutHdl);
			u32 const totalAtts = NumCast<u32>(vertAtts.Layout.Entries.size());
			u32 usedAtts = 0;
			for (u32 i=0; i<totalAtts; ++i)
			{
				DataLayoutEntry const& entry = vertAtts.Layout.Entries[i];
				//if ((VertexAttributeDesc::SemanticMap[entry.mName] & requiredAtts) == 0)
				//{
				//	continue;
				//}
				++usedAtts;
				D3D12_INPUT_ELEMENT_DESC& desc = inputElements.emplace_back();
				Zero(desc);
				desc.Format = GetFormatForType(entry.mType);
				desc.SemanticName = GetNameData(entry.mName);
				desc.AlignedByteOffset = NumCast<u32>(entry.mOffset);
				desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			}
		}
		desc.InputLayout.NumElements = NumCast<u32>(inputElements.size());
		desc.InputLayout.pInputElementDescs = Addr(inputElements);
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = PSODesc.NumRTs;
		for (u32 i = 0; i < 8; ++i)
		{
			desc.RTVFormats[i] = PSODesc.RTVFormats[i];
		}
		desc.DSVFormat = PSODesc.DSVFormat;
		desc.SampleDesc = PSODesc.SampleDesc;
		mDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
	}

	return pso.Get();
}

void DX12RHI::FreeDirectMesh(DX12DirectMesh* mesh)
{
	DeferredRelease(std::move(mesh->mResource));
	mMeshes.Release(std::move(*mesh));
}

void DX12RHI::FreeIndexedMesh(DX12IndexedMesh* mesh)
{
	DeferredRelease(std::move(mesh->VertBuff));
	DeferredRelease(std::move(mesh->IndBuff));
	mIndexedMeshes.Release(std::move(*mesh));
}

ID3D12Device_* GetD3D12Device()
{
	return sRHI->Device();
}

DX12RHI& GetRHI()
{
	return *sRHI;
}

void DX12DirectMesh::OnFullyReleased()
{
	GetRHI().FreeDirectMesh(this);
}

void DX12IndexedMesh::OnFullyReleased()
{

}

}
}
