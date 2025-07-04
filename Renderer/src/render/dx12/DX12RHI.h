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
#include "core/memory/PoolAllocator.h"
#include "render/RenderDeviceCtx.h"
#include "core/Hash.h"
#include "DX12Context.h"
#include "scene/Camera.h"
#include "dxgi1_6.h"
#include "DX12QueryHeap.h"
#include "core/memory/GrowingImmobileObjectPool.h"


DECLARE_LOG_CATEGORY(LogDX12);

namespace rnd::dx12
{
class DX12Test;
class DX12DescriptorAllocator;
class DX12ReadbackBuffer;
class DX12SwapChain;
class DX12CommandAllocatorPool;
class DX12Texture;
class DX12DescriptorTableAllocator;
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

struct DX12ReadbackAllocation
{
	ComPtr<ID3D12Resource_> Resource;
	u64 Offset;
	u64 Size;
};

struct DX12ReadbackBufferEntry
{
	ID3D12Resource_* Buffer;
	u64 BufferOffset;
	void const* Data;
	u64 Size;
};

class DX12RHI : //public wnd::Window,
public IRenderDevice
{
public:
	static DX12RHI* InitRHI();
	static bool IsCreated();
	
	DX12RHI(u32 width, u32 height, wchar_t const* name, ESwapchainBufferCount numBuffers, Scene* scene = nullptr, Camera::Ref camera = nullptr);
	~DX12RHI();

	char const* GetName() const override
	{
		return "D3D12";
	}

	void ImGuiInit(wnd::Window* mainWindow);
	void ImGuiBeginFrame();
	void ImGuiEndFrame();

	void BeginFrame() override;
	void Tick();

	void ProcessQueries(u32 frameIdx);

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

	constexpr static u32 ReadbackCleanupDelay = 1;
	// readbackDelay = number of frames to keep valid plus the usual num frames in flight
	DX12ReadbackBufferEntry AllocateReadback(u64 size, u64 alignment, u32 readbackDelay);
	DX12ReadbackAllocation AllocateReadbackBuffer(u64 size);
	void			   FreeReadback(DX12ReadbackAllocation& buffer);

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

	IDeviceSurface* CreateSurface(wnd::Window* window) override;
	GPUTimer*		CreateTimer(wchar_t const* name);
	DX12QueryHeap* GetTimingQueries() const
	{
		return mTimingQueries.get();
	}

	void ProcessTimers();

private:
	void WaitFence(u64 value);

	void StartFrame();
	void WaitAndReleaseFrameIdx(u32 frameIdx);
	void WaitAndReleaseFrame(u64 frame);
	void EndFrame();

	void CreateDeviceAndCmdQueue();
	bool ResizeSwapChains();

	ComPtr<ID3D12Resource> CreateVertexBuffer(VertexBufferData data, ID3D12GraphicsCommandList_* uploadCmdList);

	PoolAllocator<DX12DirectMesh, 1024, true> mMeshes;
	PoolAllocator<DX12IndexedMesh, 2048, true> mIndexedMeshes;
	DX12Uploader mUploader;
	DX12CommandQueues mQueues;
	ComPtr<ID3D12Device_> mDevice; 
	ComPtr<ID3D12Fence> mFrameFence;
	HANDLE mFenceEvent{};
	u64 mCurrentFrame = 0;
	std::array<u64, MaxSwapchainBufferCount> mFenceValues = {0,0,0};
	OwningPtr<DX12Test> mTest;
	OwningPtr<DX12Allocator> mSimpleAllocator;
	OwningPtr<DX12Allocator> mReadbackAllocator;
	OwningPtr<DX12UploadBuffer> mUploadBuffer;
	OwningPtr<DX12ReadbackBuffer> mReadbackBuffer;
	OwningPtr<DX12DescriptorAllocator> mShaderResourceDescAlloc;
	OwningPtr<DX12DescriptorAllocator> mRTVAlloc;
	OwningPtr<DX12DescriptorAllocator> mDSVAlloc;
	OwningPtr<DX12DescriptorTableAllocator> mShaderResourceDescTables;
	OwningPtr<DX12DescriptorTableAllocator> mSamplerDescTables;
	OwningPtr<DX12QueryHeap> mTimingQueries;
	DX12DescriptorHeap mImguiHeap;
	u32 mImGuiHeapIndex = 0;
	std::array<OwningPtr<DX12CommandAllocatorPool>, 3> mCmdAllocatorPools;
	std::array<Vector<ComPtr<ID3D12Pageable>>, MaxSwapchainBufferCount> mDeferredReleaseResources;
	std::unordered_map<GraphicsPSODesc, ComPtr<ID3D12PipelineState>, GenericHash<>> mPSOs;
	std::unordered_map<ComputePSODesc, ComPtr<ID3D12PipelineState>, GenericHash<>> mComputePSOs;
	DX12DescriptorHeap mRTVHeap;
	DX12CommandList mCmdList;
	ESwapchainBufferCount mNumBuffers;
	bool mClosed = false;
	bool mRecordingCommands = false;
	u8 mFrameIndex = 0;
	u8 mStartedFrameIndex = 0;
	OwningPtr<DX12Context> mContext;
	Scene* mScene = nullptr;
	DX12SwapChain* mImGuiMainWindow = nullptr;

	DX12CBPool mCBPool;
	ComPtr<IDXGIFactory6> mFactory;
	SmallVector<OwningPtr<DX12SwapChain>, 4> mSwapChains;

	GrowingImmobileObjectPool<DX12Timer, 256, true> mTimerPool;
};

using DX12RHI = DX12RHI;

ID3D12Device_* GetD3D12Device();
DX12RHI& GetRHI();

}
