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
#include "render/RenderController.h"
#include "editor/Viewport.h"
#include "render/RendererScene.h"
#include "imgui.h"
#include "DX12CommandAllocatorPool.h"
#include "DX12SwapChain.h"
#include "render/shaders/ShaderDeclarations.h"

#pragma comment(lib, "d3d12.lib")

DEFINE_LOG_CATEGORY(LogDX12);

namespace rnd
{
namespace dx12
{

static DX12RHI* sRHI = nullptr;

DX12RHI::DX12RHI(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers, Scene* scene, Camera::Ref camera)
:IRenderDevice(new DX12LegacyShaderCompiler), mNumBuffers(numBuffers)
{
	GRenderController.AddRenderBackend(this);

	ZE_ASSERT(sRHI == nullptr);
	sRHI = this;

	CreateDeviceAndCmdQueue();
	DX12DescriptorHeap::GetDescriptorSizes(mDevice.Get());


	DX12SyncPointPool::Create(mDevice.Get());
	mSimpleAllocator = MakeOwning<DX12Allocator>(mDevice.Get());
	mReadbackAllocator = MakeOwning<DX12Allocator>(mDevice.Get(), D3D12_HEAP_TYPE_READBACK);

	mFenceEvent = CreateEventW(nullptr, false, false, nullptr);
	mCBPool = DX12CBPool(*this);
	mUploadBuffer = MakeOwning<DX12UploadBuffer>(1024 * 1024 * 256);
	mUploader = DX12Uploader(mDevice.Get());

	mShaderResourceDescAlloc = MakeOwning<DX12SeparableDescriptorAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64 * 1024, false);
	mRTVAlloc = MakeOwning<DX12SeparableDescriptorAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64, false);
	mDSVAlloc = MakeOwning<DX12SeparableDescriptorAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64, false);
	mShaderResourceDescTables = MakeOwning<DX12DescriptorTableAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mSamplerDescTables = MakeOwning<DX12DescriptorTableAllocator>(Device(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

//	mSwapChains.push_back(MakeOwning<DX12SwapChain>(this, (u32)mNumBuffers));

	if (scene)
	{
		mScene = scene;
		mContext = std::make_unique<DX12Context>(mCmdList);
//		mViewport = mSwapChains[0]->CreateFullscreenViewport(mScene, camera);
	}
	{
		mTest = MakeOwning<DX12Test>(mDevice.Get());
	}
	mCmdList.CmdList->Reset(mCmdList.Allocator.Get(), mTest->mPSO.Get());

	WaitForGPU();
	ResourceMgr.OnDevicesReady();

}

DX12RHI::~DX12RHI()
{
	mSwapChains.clear();
	mViewport = nullptr;
	mContext = nullptr;
		//	mSwapchainBufferTextures = {};
	ResourceMgr.Teardown();
	MatMgr.Release();
	BasicTextures.Teardown();
	BasicMeshes.Teardown();
	GRenderController.RemoveRenderBackend(this);
	RendererScene::OnShutdownDevice(this);
	mTest = nullptr;
	mUploadBuffer = nullptr;
	mCBPool.ReleaseResources();
	DX12SyncPointPool::Destroy();
	mRTVAlloc = nullptr;
	mDSVAlloc = nullptr;
	mShaderResourceDescAlloc = nullptr;

	WaitFence(mCurrentFrame - 1);
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
	ResizeSwapChains();

	//if (mViewport && mViewport->mRCtx)
	//{
	//	ImGui::Begin("DX12");
	//	mViewport->mRCtx->DrawControls();
	//	ImGui::End();
	//}


	StartFrame();
	auto& cmd = mCmdList.CmdList;
//	cmd->Reset(mCmdList.Allocators[mFrameIndex].Get(), );
	mRecordingCommands = true;

	//D3D12_VIEWPORT vp {};
	//vp.Height = float(mHeight);
	//vp.Width = float(mWidth);
	//vp.TopLeftX = vp.TopLeftY = 0;
	//vp.MaxDepth = 1;
	//vp.MinDepth = 0;
	//cmd->RSSetViewports(1, &vp);
	//CD3DX12_RECT scissor(0, 0, mWidth, mHeight);
	//cmd->RSSetScissorRects(1, &scissor);

	for (auto& swapChain : mSwapChains)
	{
		swapChain->GetCurrentBuffer()->TransitionTo(cmd.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	
	if (!mSwapChains.empty())
	{
		
		D3D12_CPU_DESCRIPTOR_HANDLE rtv 
			= mSwapChains[0]->GetCurrentBuffer()->GetRT()->GetData<D3D12_CPU_DESCRIPTOR_HANDLE>();
		cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
		float const clearColor[4] = {0.5f, 0.5f, 0.5f, 1.f};
		cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		if (mTest)
		{
	//		mTest->Render(*cmd.Get());
		}
		if (mViewport)
		{
			mContext->ClearRenderTarget(mSwapChains[0]->GetCurrentBuffer()->GetRT(), {0,0,0,0});
			if (!mViewport->mRScene->IsInitialized())
			{
				mViewport->mRScene = RendererScene::Get(*mScene, mContext.get());
			}
			mViewport->SetBackbuffer(mSwapChains[0]->GetCurrentBuffer());
		}
	}

//	EndFrame();
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
	//if (mFenceValue > 2)
	//{

	//}
}

void DX12RHI::EndFrame()
{
	for (auto const& swapChain : mSwapChains)
	{
		swapChain->GetCurrentBuffer()->TransitionTo(mCmdList.CmdList.Get(), D3D12_RESOURCE_STATE_PRESENT);
	}
	mRecordingCommands = false;
	mCmdList.CmdList->Close();
	ID3D12CommandList* const cmdList = mCmdList.CmdList.Get();
	mQueues.Direct->ExecuteCommandLists(1, &cmdList);
	mCmdList.Reset(mTest->mPSO.Get());

	for (auto const& swapChain : mSwapChains)
	{
		swapChain->Present();
	}
	mFenceValues[mFrameIndex] = mCurrentFrame;

	HR_ERR_CHECK(mQueues.Direct->Signal(mFrameFence.Get(), mCurrentFrame));
	++mCurrentFrame;
	mFrameIndex = (mCurrentFrame) % mNumBuffers;
	WaitAndReleaseFrameIdx(mFrameIndex);
}

void DX12RHI::WaitForGPU()
{
	DX12SyncPoint syncPoint = DX12SyncPointPool::Get()->Claim();
	syncPoint.GPUSignal(mQueues.Direct.Get());
	syncPoint.WaitAndRelease();

}

void DX12RHI::DeferredRelease(ComPtr<ID3D12Pageable>&& resource)
{
	mDeferredReleaseResources[mFrameIndex].emplace_back(std::move(resource));
}

void DX12RHI::ProcessDeferredRelease(u32 frameIndex)
{
	mDeferredReleaseResources[frameIndex].clear();
	for (auto& pool : mCmdAllocatorPools)
	{
		pool->ProcessDeferredReleases(frameIndex);
	}
}

//void DX12RHI::Resize_WndProc(u32 resizeWidth, u32 resizeHeight)
//{
//	Window::Resize_WndProc(resizeWidth, resizeHeight);
//	mResizeWidth = resizeWidth;
//	mResizeHeight = resizeHeight;
//}

//void DX12RHI::OnDestroy_WndProc()
//{
//	wnd::Window::OnDestroy_WndProc();
//	mClosed = true;
//}

void DX12RHI::CreateDeviceAndCmdQueue()
{
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug1> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(true);
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif


	HR_ERR_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

	ComPtr<IDXGIAdapter1> adapter;

	for (u32 adapterIdx = 0; SUCCEEDED(mFactory->EnumAdapterByGpuPreference(adapterIdx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); ++adapterIdx)
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

	mQueues.Direct = DX12CommandQueue(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	mQueues.Copy = DX12CommandQueue(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COPY);
	mQueues.Compute = DX12CommandQueue(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);

	mCmdAllocatorPools[0] = MakeOwning<DX12CommandAllocatorPool>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mCmdAllocatorPools[1] = MakeOwning<DX12CommandAllocatorPool>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCmdAllocatorPools[2] = MakeOwning<DX12CommandAllocatorPool>(D3D12_COMMAND_LIST_TYPE_COPY);


	// Create command list + allocators
	HR_ERR_CHECK(mDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCmdList.CmdList)));
	HR_ERR_CHECK(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdList.Allocator)));

	mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence));
}

bool DX12RHI::ResizeSwapChains()
{
	bool bResizeNeeded = false;
	for (auto& swapChain : mSwapChains)
	{
		if (swapChain->IsResizeRequested())
		{
			bResizeNeeded = true;
			swapChain->ReleaseResourcesPreResize();
		}
	}
	if (!bResizeNeeded)
	{
		return false;
	}
	WaitForGPU();
	mRTVHeap = {};
	//mWidth = mResizeWidth;
	//mHeight = mResizeHeight;
//	mResizeWidth = mResizeHeight = 0;
	mDeferredReleaseResources = {};

	for (auto& swapChain : mSwapChains)
	{
		if (swapChain->IsResizeRequested())
		{
			swapChain->Resize();
		}
	}
	return true;

}

ComPtr<ID3D12Resource> DX12RHI::CreateVertexBuffer(VertexBufferData data, ID3D12GraphicsCommandList_* uploadCmdList)
{

	constexpr u32 VertexBufferAlignment = 4;
	u32 size = NumCast<u32>(data.VertSize * data.NumVerts);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_NONE, VertexBufferAlignment);
	auto finalLocation = GetAllocator(EResourceType::VertexBuffer, data.VertSize * data.NumVerts)->Allocate(desc, D3D12_RESOURCE_STATE_COMMON, nullptr);
	
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		finalLocation.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	uploadCmdList->ResourceBarrier(1, &barrier);
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
	result->View.BufferLocation = result->mResource->GetGPUVirtualAddress();
	
	result->View.SizeInBytes = size;
	result->View.StrideInBytes = data.VertSize;
	uploadCtx.FinishUploadSynchronous();
//	mDevice->create
//	mUploader.
//	UpdateSubresources()
	return result;
}

RefPtr<IDeviceIndexedMesh> DX12RHI::CreateIndexedMesh(EPrimitiveTopology topology, VertexBufferData vertexBuffer, Span<u16> indexBuffer, BatchedUploadHandle uploadHandle)
{
	u32 vbSize = NumCast<u32>(vertexBuffer.VertSize * vertexBuffer.NumVerts);
	auto uploadCtx = mUploader.StartUpload();
	auto* result = mIndexedMeshes.Acquire();
	result->VertBuff = CreateVertexBuffer(vertexBuffer, uploadCtx.CmdList.Get());
	result->VertBuffView.BufferLocation = result->VertBuff->GetGPUVirtualAddress();
	
	result->VertBuffView.SizeInBytes = vbSize;
	result->VertBuffView.StrideInBytes = vertexBuffer.VertSize;

	constexpr u32 IndexBufferAlignment = 4;
	u32 ibSize = NumCast<u32>(indexBuffer.size() * sizeof(u16));
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(ibSize, D3D12_RESOURCE_FLAG_NONE, IndexBufferAlignment);
	auto ibResource = GetAllocator(EResourceType::VertexBuffer, ibSize)->Allocate(desc, D3D12_RESOURCE_STATE_COMMON, nullptr);
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		ibResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	uploadCtx.CmdList->ResourceBarrier(1, &barrier);

	auto uploadLoc = mUploadBuffer->Reserve(ibSize, IndexBufferAlignment);
	memcpy(uploadLoc.WriteAddress, indexBuffer.data(), ibSize);;
	uploadCtx.CmdList->CopyBufferRegion(ibResource.Get(), 0, mUploadBuffer->GetCurrentBuffer(), uploadLoc.Offset, ibSize);

	result->IndBuff = ibResource;
	result->IndBuffView.BufferLocation = ibResource->GetGPUVirtualAddress();
	result->IndBuffView.Format = DXGI_FORMAT_R16_UINT;
	result->IndBuffView.SizeInBytes = ibSize;
	result->IndexCount = NumCast<u32>(indexBuffer.size());

	uploadCtx.FinishUploadSynchronous();
	return result;
}


bool DX12RHI::ShouldDirectUpload(EResourceType resourceType, u64 size)
{
	return IsInRenderThread() && IsRecordingCommands();
}

IDeviceTexture::Ref DX12RHI::CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData /*= nullptr*/)
{
	DeviceTextureDesc realDesc = desc;
	if (desc.NumMips == 0)
	{
		realDesc.Flags |= TF_UAV;
		realDesc.NumMips = max(RoundUpLog2(desc.Width), RoundUpLog2(desc.Height));
	}
	auto result = std::make_shared<DX12Texture>(realDesc);
	if (!initialData)
	{
		return result;
	}
	ZE_ASSERT(desc.SampleCount == 1);

	u64 uploadSize = GetRequiredIntermediateSize(result->GetTextureHandle<ID3D12Resource*>(), 0, 1); // TODO mips
	auto alloc = mUploadBuffer->Reserve(uploadSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	D3D12_SUBRESOURCE_DATA srcData{};
	srcData.pData = initialData;
	srcData.RowPitch = desc.GetRowPitch();

	if (ShouldDirectUpload(desc.ResourceType) || desc.NumMips == 0
	)
	{
		static u32 uploadCount = 0;
		UpdateSubresources<1>(mCmdList.CmdList.Get(), result->GetTextureHandle<ID3D12Resource*>(), mUploadBuffer->GetCurrentBuffer(), alloc.Offset,
			0u, desc.NumMips == 0 ? 1u : desc.NumMips, &srcData); // TODO mips
		result->UpdateCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
		GenerateMips(mCmdList, result);
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
IDeviceTexture::Ref DX12RHI::CreateTextureCube(DeviceTextureDesc const& inDesc, CubemapData const& initialData)
{
	DeviceTextureDesc actualDesc = inDesc;
	bool foldUp = initialData.Tex && initialData.Format == ECubemapDataFormat::FoldUp;
	u32 faceWidth = 0;
	if (foldUp)
	{
		faceWidth = initialData.Tex->width / 4;;
		ZE_ASSERT(faceWidth * 4 == initialData.Tex->width);
		ZE_ASSERT(faceWidth * 3 == initialData.Tex->height);
		actualDesc.Width = actualDesc.Height = faceWidth;
	}
	actualDesc.ResourceType = EResourceType::TextureCube;
	auto result = std::make_shared<DX12Texture>(actualDesc);
	if (foldUp)
	{
		auto& uploadCtx = mUploader.StartUpload();
		u64 uploadSize = GetRequiredIntermediateSize(result->GetTextureHandle<ID3D12Resource*>(), 0, 6); // TODO mips
		auto alloc = mUploadBuffer->Reserve(uploadSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		D3D12_SUBRESOURCE_DATA srcData[6];
		Zero(srcData);
		constexpr u32 const startPos[6] = {
			6, 4, 1, 9, 5, 7
		};
		for (u32 i=0; i<6; ++i)
		{
			srcData[i].pData = initialData.Tex->GetData() + (faceWidth * faceWidth * (startPos[i] / 4) * 4) + faceWidth * (startPos[i] % 4);
			srcData[i].RowPitch = u32(initialData.Tex->width) * sizeof(u32);
		}

		UpdateSubresources<6>(uploadCtx.CmdList.Get(), result->GetTextureHandle<ID3D12Resource*>(), mUploadBuffer->GetCurrentBuffer(),
			alloc.Offset, 0u, 6u, srcData);
		uploadCtx.FinishUploadSynchronous();
		result->UpdateCurrentState(D3D12_RESOURCE_STATE_COMMON);
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
	if (IsValid(descriptor))
	{
		GetDescriptorAllocator(descType)->Free(descriptor);
	}
}

DX12ReadbackBuffer DX12RHI::AllocateReadback(u64 size)
{
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	return { mReadbackAllocator->Allocate(desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr).Get(), 0};
}

void DX12RHI::FreeReadback(DX12ReadbackBuffer& buffer)
{
	buffer = {};// Should be safe to release it immediately...
}

DX12CommandAllocator DX12RHI::AcquireCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
	auto allocator = GetCommandAllocatorPool(type)->Acquire();
	return {allocator, type};
}

void DX12RHI::ReleaseCommandAllocator(DX12CommandAllocator&& allocator)
{
	GetCommandAllocatorPool(allocator.Type)->Release(std::move(allocator.Allocator));
}

DX12CommandAllocatorPool* DX12RHI::GetCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return mCmdAllocatorPools[0].get();
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return mCmdAllocatorPools[1].get();
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return mCmdAllocatorPools[2].get();
	default:
		ZE_ASSERT(false);
		return nullptr;
	}
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
	if (state == EBlendState::NONE)
	{
		rtb.BlendEnable = false;
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
//		CD3DX12_GRAPHICS_PIPELINE_STATE_DESC
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
		desc.pRootSignature = PSODesc.RootSig.Get();
		desc.VS = GetBytecode(PSODesc.Shaders[EShaderType::Vertex]);
		desc.PS = GetBytecode(PSODesc.Shaders[EShaderType::Pixel]);
		desc.GS = GetBytecode(PSODesc.Shaders[EShaderType::Geometry]);
		desc.DS = GetBytecode(PSODesc.Shaders[EShaderType::TessEval]);
		desc.HS = GetBytecode(PSODesc.Shaders[EShaderType::TessControl]);
		ApplyBlendState(PSODesc.BlendState, desc.BlendState);
		desc.SampleMask = UINT_MAX;
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
		desc.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT{});
//		desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		DXCALL(mDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
	}

	return pso.Get();
}

ID3D12PipelineState* DX12RHI::GetPSO(ComputePSODesc const& PSODesc)
{
	auto& pso = mComputePSOs[PSODesc];
	if (pso == nullptr)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc {};
		desc.CS = GetBytecode(PSODesc.Shader);
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		desc.pRootSignature = PSODesc.RootSig.Get();
		DXCALL(mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));
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

DX12RHI* DX12RHI::InitRHI()
{
	ZE_ASSERT(sRHI == nullptr);
	sRHI = new DX12RHI(0,0, L"", ESwapchainBufferCount::TripleBuffered);
	return sRHI;
}

bool DX12RHI::IsCreated()
{
	return sRHI != nullptr;
}

void DX12RHI::GenerateMips(DX12CommandList& cmdList, DX12Texture::Ref texture)
{
	const bool srgb = texture->Desc.Format == ETextureFormat::RGBA8_Unorm_SRGB;
	if (!srgb)
	{
		RLOG(LogDX12, Verbose, "%s is not srgb", texture->Desc.DebugName.c_str());
	}
	//else
	//{
	//	return;
	//}
	u32 lastGenerated = 0;
	SmallVector<D3D12_RESOURCE_BARRIER, 16> barriers;
	u32 numMips = texture->Desc.NumMips;
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTextureHandle<ID3D12Resource*>(),
		 D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0));
	for (u32 i=1; i<numMips; ++i)
	{
		texture->CreateUAV(i);
		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTextureHandle<ID3D12Resource*>(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, lastGenerated + i));
	}
	cmdList->ResourceBarrier(NumCast<u32>(barriers.size()), &barriers[0]);
	barriers.clear();
	u32 srvBits = 1;
	while (lastGenerated < numMips - 1)
	{
		texture->CreateSRV(lastGenerated);
		u32										numToGenerate = min(numMips - 1 - lastGenerated, DownsampleCS::MaxDownsamples);
		if (lastGenerated > 0)
		{
			barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTextureHandle<ID3D12Resource*>(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lastGenerated));
			srvBits |= (1 << lastGenerated);
			cmdList->ResourceBarrier(NumCast<u32>(barriers.size()), &barriers[0]);
			barriers.clear();
		}
		SmallVector<UnorderedAccessView, DownsampleCS::MaxDownsamples> uavs;
		for (u32 i = 1; i <= numToGenerate; ++i)
		{
			uavs.push_back({texture, i + lastGenerated});
		}

		DX12Context ctx(mCmdList, true);
		ctx.SetUAVs(EShaderType::Compute, uavs);
		ctx.SetShaderResources(EShaderType::Compute, Single<ResourceView const>({texture, lastGenerated}));
		DownsampleCS::Permutation perm;
		perm.Set<DownsampleCS::IsSrgb>(srgb);
		perm.Set<DownsampleCS::NumLevels>(numToGenerate);
		ctx.SetComputeShader(ShaderMgr.GetCompiledShader<DownsampleCS>(perm));
		uvec2 threadsRequired = DivideRoundUp(texture->Desc.Extent(), 2u << lastGenerated);
		ctx.DispatchCompute(DivideRoundUp(threadsRequired, 8u));
		lastGenerated += numToGenerate;
	}
	for (u32 i=0; i<texture->Desc.NumMips; ++i)
	{
		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTextureHandle<ID3D12Resource*>(),
			(srvBits & 1) ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, i));
		srvBits >>= 1;
	}


	cmdList->ResourceBarrier(NumCast<u32>(barriers.size()), &barriers[0]);
	texture->UpdateCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	

}

void DX12RHI::ExecuteCommand(std::function<void(IRenderDeviceCtx&)>&& command, char const* name)
{
	DX12Context context(mCmdList);
	command(context);
}

bool DX12RHI::GetImmediateContext(std::function<void(IRenderDeviceCtx&)> callback)
{
	DX12Context ctx(mCmdList);
	callback(ctx);
	return true;
}

//void DX12RHI::Move_WndProc(int xPos, int yPos)
//{
//	Window::Move_WndProc(xPos, yPos);
//	mViewport->UpdatePos({xPos, yPos});
//}

ID3D12PipelineState* DX12RHI::GetDefaultPSO()
{
	return mTest->mPSO.Get();
}

void DX12RHI::BeginFrame()
{
	IRenderDevice::BeginFrame();
	Tick();
}

rnd::IDeviceSurface* DX12RHI::CreateSurface(wnd::Window* window)
{
	return mSwapChains.emplace_back(MakeOwning<DX12SwapChain>(window, mNumBuffers)).get();
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
	GetRHI().FreeIndexedMesh(this);
}

void DX12CommandList::Reset(ID3D12PipelineState* pso)
{
	GetRHI().ReleaseCommandAllocator({Allocator, Type});
	Allocator = GetRHI().AcquireCommandAllocator(Type).Allocator;
	CmdList->Reset(Allocator.Get(), pso);
}

}
}
