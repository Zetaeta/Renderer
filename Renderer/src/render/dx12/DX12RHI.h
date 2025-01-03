#pragma once
#include "core/Maths.h"
#include "core/WinUtils.h"
#include "core/Types.h"
#include "platform/windows/Window.h"
#include "platform/windows/WinApi.h"
#include "SharedD3D12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <array>
#include "DX12DescriptorHeap.h"
#include "DX12SyncPoint.h"
#include "DX12CBPool.h"
#include "DX12CommandQueue.h"
#include "DX12Upload.h"
#include "render/ShaderManager.h"
#include "core/memory/GrowingImmobilePool.h"
#include "render/RenderDeviceCtx.h"

namespace rnd { namespace dx12 { class DX12DescriptorTableAllocator; } }


namespace rnd::dx12
{
class DX12Test;
class DX12DescriptorAllocator;
class DX12UploadBuffer;
enum class EDescriptorType : u8;

struct DX12CommandList
{
	ComPtr<ID3D12GraphicsCommandList_> CmdList;
	std::array<ComPtr<ID3D12CommandAllocator>, MaxSwapchainBufferCount> Allocators;
};

struct DX12DirectMesh : public IDeviceMesh
{
	ComPtr<ID3D12Resource> mResource;
	D3D12_VERTEX_BUFFER_VIEW view;
};

class DX12RHI : public wnd::Window, public IRenderDevice
{
public:
	DX12RHI(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers);
	~DX12RHI();

	void Tick() override;

	u64 GetCompletedFrame() const;
	u64 GetCurrentFrame() const
	{
		return mCurrentFrame;
	}

	// Returns current frame % num buffers
	u32 GetCurrentFrameIndex() const
	{
		return mFrameIndex;
	}
	void WaitForGPU();

	void DeferredRelease(ComPtr<ID3D12Pageable>&& resource);

	void ProcessDeferredRelease(u32 frameIndex);

	ID3D12Device_* Device() const
	{
		return mDevice.Get();
	}

	u32 GetMaxActiveFrames() const
	{
		return mNumBuffers;
	}

	DX12CBPool& GetCBPool()
	{
		return mCBPool;
	}


	DX12CommandQueues& CmdQueues()
	{
		return mQueues;
	}


	DX12Uploader& GetUploader()
	{
		return mUploader;
	}

	DX12Allocator* GetAllocator(EResourceType resourceType, u64 size = (u64) -1)
	{
		return mSimpleAllocator.get();
	}

	bool ShouldDirectUpload(EResourceType resourceType, u64 size = (u64) -1);

	// Temp: are we recording commands on main command list?
	bool IsRecordingCommands()
	{
		return mRecordingCommands;
	}
	

	virtual IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData = CubemapData{}) override { return nullptr;}
	virtual IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData = nullptr) override;
	virtual DeviceMeshRef		CreateDirectMesh(VertexAttributeDesc::Handle vertAtts, u32 numVerts, u32 vertSize, void const* data) override;

	virtual SamplerHandle GetSampler(SamplerDesc const& desc) { return {0}; }

	DX12DescriptorAllocator* GetDescriptorAllocator(EDescriptorType descType);

	DX12DescriptorTableAllocator* GetResourceDescTableAlloc()
	{
		return mShaderResourceDescTables.get();
	}
	DX12DescriptorTableAllocator* GetSamplerTableAlloc()
	{
		return mSamplerDescTables.get();
	}
private:
	void WaitFence(u64 value);

	void StartFrame();
	void EndFrame();
	void Resize_WndProc(u32 resizeWidth, u32 resizeHeight) override;
	void OnDestroy_WndProc() override;

	void CreateDeviceAndCmdQueue();
	void ResizeSwapChain();
	void GetSwapChainBuffers();

	GrowingImmobilePool<DX12DirectMesh, 1024, true> mMeshes;
	ShaderManager mShaderMgr;
	DX12Uploader mUploader;
	DX12CommandQueues mQueues;
	ComPtr<ID3D12Device_> mDevice; 
	ComPtr<IDXGISwapChain3> mSwapChain; 
	ComPtr<ID3D12Fence> mFrameFence;
	HANDLE mFenceEvent{};
	u64 mCurrentFrame = 0;
	std::array<u64, MaxSwapchainBufferCount> mFenceValues = {0,0,0};
	OwningPtr<DX12Test> mTest;
	OwningPtr<DX12Allocator> mSimpleAllocator;
	OwningPtr<DX12UploadBuffer> mUploadBuffer;
	OwningPtr<DX12DescriptorAllocator> mShaderResourceDescAlloc;
	OwningPtr<DX12DescriptorTableAllocator> mShaderResourceDescTables;
	OwningPtr<DX12DescriptorTableAllocator> mSamplerDescTables;
//	OwningPtr<DX12SyncPointPool> mSyncPoints;
	std::array<ComPtr<ID3D12Resource_>, MaxSwapchainBufferCount> mRenderTargets;
	std::array<Vector<ComPtr<ID3D12Pageable>>, MaxSwapchainBufferCount> mDeferredReleaseResources;
	DX12DescriptorHeap mRTVHeap;
	DX12CommandList mCmdList;
	ESwapchainBufferCount mNumBuffers;
	bool mClosed = false;
	bool mRecordingCommands = false;
	u32 mFrameIndex = 0;
	u32 mBufferIndex = 0;
	u32 mResizeWidth = 0;
	u32 mResizeHeight = 0;

	DX12CBPool mCBPool;
};

using DX12RHI = DX12RHI;

ID3D12Device_* GetD3D12Device();
DX12RHI& GetRHI();

}
