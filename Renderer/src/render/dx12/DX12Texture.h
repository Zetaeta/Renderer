#pragma once

#include "SharedD3D12.h"
#include "render/DeviceTexture.h"
#include "container/Vector.h"

#include <variant>

namespace rnd::dx12
{
class DX12RenderTarget;
class DX12DepthStencil;
struct EmptyStruct
{
};


class DX12DepthStencil : public IDepthStencil
{
public:
	DX12DepthStencil(DepthStencilDesc const& desc, DX12Texture* tex, ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const& dxDesc);
	DX12DepthStencil(DX12DepthStencil&& other);
	DX12DepthStencil& operator=(DX12DepthStencil&& other);
	~DX12DepthStencil();

	OpaqueData<8> GetData() const override { return mDesc[0]; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle(u32 idx = 0) const
	{
		return mDesc[idx];
	}

	DX12Texture* BaseTex = nullptr;
private:
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> mDesc{};
//	ComPtr<ID3D12Resource> mResource;

};

class DX12RenderTarget : public IRenderTarget
{
public:
	DX12RenderTarget(RenderTargetDesc const& desc, DX12Texture* tex, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const& dxDesc);
	DX12RenderTarget(RenderTargetDesc const& desc, DX12Texture* tex, ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE existingRtvDesc);
	DX12RenderTarget(DX12RenderTarget&& other);
	DX12RenderTarget& operator=(DX12RenderTarget&& other);
	~DX12RenderTarget();
	OpaqueData<8> GetRTData() override { return DescriptorHdl[0]; }
	OpaqueData<8> GetData() const override { return DescriptorHdl[0]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle(u32 idx = 0) const
	{
		return DescriptorHdl[idx];
	}
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> DescriptorHdl{};
//	ComPtr<ID3D12Resource> mResource;

	DX12Texture* BaseTex = nullptr;
};

class DX12Texture : public IDeviceTexture//, public std::enable_shared_from_this<DX12Texture>
{
public:
	struct UploadTools
	{
		DX12UploadBuffer& Buffer;
		ID3D12GraphicsCommandList_* UploadCmdList;
	};
	using Ref=std::shared_ptr<DX12Texture>;

	DX12Texture(DeviceTextureDesc const& desc, TextureData data, UploadTools& uploadData);
	DX12Texture(DeviceTextureDesc const& desc);
	DX12Texture(DeviceTextureDesc const& desc, ID3D12Resource_* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	~DX12Texture();

	OpaqueData<8> GetRHIResource() const override
	{
		return mResource.Get();
	}

	using IDeviceTexture::GetTextureHandle;
	OpaqueData<8> GetTextureHandle() const override;
	OpaqueData<8> GetData() const override;
	IRenderTarget::Ref GetRT() override;
	IDepthStencil::Ref GetDS() override;
	void CreateSRV(u32 srvIndex) override;
	MappedResource Map(u32 subResource, ECpuAccessFlags flags) override;
	void Unmap(u32 subResource) override;
	CopyableMemory<8> GetShaderResource(ShaderResourceId id) override;
	OpaqueData<8> GetUAV(UavId id) override;

	UavId CreateUAV(u32 subresourceIdx) override;

	void TransitionState(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState);
	void TransitionTo(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES toState);
	void UpdateCurrentState(D3D12_RESOURCE_STATES inState)
	{
		mLastState = inState;
	}
	D3D12_RESOURCE_STATES GetCurrentState() const
	{
		return mLastState;
	}

private:
	D3D12_RESOURCE_DESC MakeResourceDesc();
	// Returns offset in upload buffer if uploading
	u64 CreateResource(_In_opt_ UploadTools* upload);

	ComPtr<ID3D12Resource> mResource;
	D3D12_CPU_DESCRIPTOR_HANDLE mSrvDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE mStencilSrvDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE mUavDesc{};
	SmallVector<D3D12_CPU_DESCRIPTOR_HANDLE, 5> mAdditionalUAVs;
	Vector<D3D12_CPU_DESCRIPTOR_HANDLE> mAdditionalSRVs;
	D3D12_RESOURCE_STATES mLastState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES mUsedReadStates {};
	//IDepthStencil::Ref mDsv;
	//IRenderTarget::Ref mRtv;
	std::variant<EmptyStruct, DX12DepthStencil, DX12RenderTarget> mRtvOrDsv;
	u64 mResourceSize = 0;

};

}
