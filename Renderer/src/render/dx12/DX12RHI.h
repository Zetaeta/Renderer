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
#include "core/Hash.h"
#include "DX12Context.h"
#include "scene/Camera.h"
#include "dxgi1_6.h"

namespace rnd { namespace dx12 { class DX12SwapChain; } }

namespace rnd { namespace dx12 { class DX12CommandAllocatorPool; } }

namespace rnd { namespace dx12 { class DX12Texture; } }

namespace rnd { namespace dx12 { class DX12DescriptorTableAllocator; } }

DECLARE_LOG_CATEGORY(LogDX12);

namespace rnd::dx12
{
class DX12Test;
class DX12DescriptorAllocator;
class DX12UploadBuffer;
enum class EDescriptorType : u8;

struct DX12CommandAllocator
{
	ComPtr<ID3D12CommandAllocator> Allocator;
	D3D12_COMMAND_LIST_TYPE Type;
	ID3D12CommandAllocator* operator->() const
	{
		return Allocator.Get();
	}

};

struct DX12CommandList
{
	ComPtr<ID3D12GraphicsCommandList_> CmdList;
	void							   Reset(ID3D12PipelineState* pso);
	ComPtr<ID3D12CommandAllocator> Allocator;
	D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ID3D12GraphicsCommandList_* Get() const
	{
		return CmdList.Get();
	}
	operator ID3D12GraphicsCommandList_* () const
	{
		return CmdList.Get();
	}
	ID3D12GraphicsCommandList_* operator->() const
	{
		return CmdList.Get();
	}
};
struct DX12DirectMesh : public IDeviceMesh
{
	void OnFullyReleased() override;
	ComPtr<ID3D12Resource> mResource;
	D3D12_VERTEX_BUFFER_VIEW View;
};

struct DX12IndexedMesh : public IDeviceIndexedMesh
{
	void OnFullyReleased() override;
	ComPtr<ID3D12Resource> VertBuff;
	D3D12_VERTEX_BUFFER_VIEW VertBuffView;
	ComPtr<ID3D12Resource> IndBuff;
	D3D12_INDEX_BUFFER_VIEW IndBuffView;
};

struct DX12ReadbackBuffer
{
	ComPtr<ID3D12Resource_> Resource;
	u64 Offset;
};

class DX12RHI : public wnd::Window, public IRenderDevice
{
public:
	static DX12RHI* InitRHI();
	static bool IsCreated();
	
	DX12RHI(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers, Scene* scene = nullptr, Camera::Ref camera = nullptr);
	~DX12RHI();

	void BeginFrame() override;
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

	void ExecuteCommand(std::function<void(IRenderDeviceCtx & )>&& command, char const* name) override;

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

	DX12ReadbackBuffer AllocateReadback(u64 size);
	void			   FreeReadback(DX12ReadbackBuffer& buffer);

	bool ShouldDirectUpload(EResourceType resourceType, u64 size = (u64) -1);

	// Temp: are we recording commands on main command list?
	bool IsRecordingCommands()
	{
		return mRecordingCommands;
	}
	

	virtual IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData = CubemapData{}) override;
	virtual IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData = nullptr) override;
	virtual DeviceMeshRef		CreateDirectMesh(EPrimitiveTopology topology, VertexBufferData data, BatchedUploadHandle uploadHandle) override;
	RefPtr<IDeviceIndexedMesh> CreateIndexedMesh(EPrimitiveTopology topology, VertexBufferData vertexBuffer, Span<u16> indexBuffer, BatchedUploadHandle uploadHandle) override;

	virtual SamplerHandle GetSampler(SamplerDesc const& desc) { return {0}; }

	DX12DescriptorAllocator* GetDescriptorAllocator(EDescriptorType descType);

	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(EDescriptorType descType);
	void FreeDescriptor(EDescriptorType descType, D3D12_CPU_DESCRIPTOR_HANDLE descriptor); 

	DX12DescriptorTableAllocator* GetResourceDescTableAlloc()
	{
		return mShaderResourceDescTables.get();
	}
	DX12DescriptorTableAllocator* GetSamplerTableAlloc()
	{
		return mSamplerDescTables.get();
	}


	class LiveObjectReporter
	{
	public:
		~LiveObjectReporter()
		{
			Execute();
		}
		ComPtr<ID3D12DebugDevice1> DebugDevice;
		void Execute()
		{
			if (DebugDevice)
			{
				DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL | D3D12_RLDO_DETAIL);
				DebugDevice = nullptr;
			}
		}
	};

	OwningPtr<LiveObjectReporter> GetLiveObjectReporter();

	ID3D12PipelineState* GetPSO(GraphicsPSODesc const& PSODesc);
	ID3D12PipelineState* GetPSO(ComputePSODesc const& PSODesc);

	void FreeDirectMesh(DX12DirectMesh* mesh);
	void FreeIndexedMesh(DX12IndexedMesh* mesh);

	DX12CommandAllocator AcquireCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
	void ReleaseCommandAllocator(DX12CommandAllocator&& allocator);

	DX12CommandAllocatorPool* GetCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);

	void GenerateMips(DX12CommandList& cmdList, DX12TextureRef texture);

	bool GetImmediateContext(std::function<void(IRenderDeviceCtx&)> callback);
	IRenderDeviceCtx* GetPersistentCtx()
	{
		return mContext.get();
	}

	ID3D12PipelineState* GetDefaultPSO();

	IDXGIFactory6* GetFactory() const
	{
		return mFactory.Get();
	}

private:
	void WaitFence(u64 value);

	void StartFrame();
	void WaitAndReleaseFrameIdx(u32 frameIdx);
	void WaitAndReleaseFrame(u64 frame);
	void EndFrame();
	void Resize_WndProc(u32 resizeWidth, u32 resizeHeight) override;
	void Move_WndProc(int resizeWidth, int resizeHeight) override;
	void OnDestroy_WndProc() override;

	void CreateDeviceAndCmdQueue();
	void ResizeSwapChain();
	void GetSwapChainBuffers();

	ComPtr<ID3D12Resource> CreateVertexBuffer(VertexBufferData data, ID3D12GraphicsCommandList_* uploadCmdList);

	GrowingImmobilePool<DX12DirectMesh, 1024, true> mMeshes;
	GrowingImmobilePool<DX12IndexedMesh, 2048, true> mIndexedMeshes;
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
	OwningPtr<DX12Allocator> mReadbackAllocator;
	OwningPtr<DX12UploadBuffer> mUploadBuffer;
	OwningPtr<DX12DescriptorAllocator> mShaderResourceDescAlloc;
	OwningPtr<DX12DescriptorAllocator> mRTVAlloc;
	OwningPtr<DX12DescriptorAllocator> mDSVAlloc;
	OwningPtr<DX12DescriptorTableAllocator> mShaderResourceDescTables;
	OwningPtr<DX12DescriptorTableAllocator> mSamplerDescTables;
	std::array<OwningPtr<DX12CommandAllocatorPool>, 3> mCmdAllocatorPools;
//	OwningPtr<DX12SyncPointPool> mSyncPoints;
	std::array<ComPtr<ID3D12Resource_>, MaxSwapchainBufferCount> mRenderTargets;
	std::array<std::shared_ptr<DX12Texture>, MaxSwapchainBufferCount> mSwapchainBufferTextures;
	std::array<Vector<ComPtr<ID3D12Pageable>>, MaxSwapchainBufferCount> mDeferredReleaseResources;
	std::unordered_map<GraphicsPSODesc, ComPtr<ID3D12PipelineState>, GenericHash<>> mPSOs;
	std::unordered_map<ComputePSODesc, ComPtr<ID3D12PipelineState>, GenericHash<>> mComputePSOs;
	DX12DescriptorHeap mRTVHeap;
	DX12CommandList mCmdList;
	ESwapchainBufferCount mNumBuffers;
	bool mClosed = false;
	bool mRecordingCommands = false;
	u32 mFrameIndex = 0;
	u32 mBufferIndex = 0;
	u32 mResizeWidth = 0;
	u32 mResizeHeight = 0;
	OwningPtr<Viewport> mViewport;
	OwningPtr<DX12Context> mContext;
	Scene* mScene = nullptr;

	DX12CBPool mCBPool;
	ComPtr<IDXGIFactory6> mFactory;
	SmallVector<OwningPtr<DX12SwapChain>, 4> mSwapChains;
};

using DX12RHI = DX12RHI;

ID3D12Device_* GetD3D12Device();
DX12RHI& GetRHI();

}
