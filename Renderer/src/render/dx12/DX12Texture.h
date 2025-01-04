#pragma once

#include "SharedD3D12.h"
#include "render/DeviceTexture.h"

namespace rnd { namespace dx12 { class DX12UploadBuffer; } }

namespace rnd::dx12
{
class DX12RenderTarget;
class DX12DepthStencil;

class DX12Texture : public IDeviceTexture
{
public:
	struct UploadTools
	{
		DX12UploadBuffer& Buffer;
		ID3D12GraphicsCommandList_* UploadCmdList;
	};

	DX12Texture(DeviceTextureDesc const& desc, TextureData data, UploadTools& uploadData);
	DX12Texture(DeviceTextureDesc const& desc);
	~DX12Texture();


	using IDeviceTexture::GetTextureHandle;
	OpaqueData<8> GetTextureHandle() const override;
	OpaqueData<8> GetData() const override;
	IRenderTarget::Ref GetRT() override;
	IDepthStencil::Ref GetDS() override;
	void CreateSRV() override;
	MappedResource Map(u32 subResource, ECpuAccessFlags flags) override;
	void Unmap(u32 subResource) override;
	CopyableMemory<8> GetShaderResource(ShaderResourceId id) override;
	OpaqueData<8> GetUAV(UavId id) override;

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
	D3D12_RESOURCE_STATES mLastState;
	IDepthStencil::Ref mDsv;
	IRenderTarget::Ref mRtv;
	u64 mResourceSize = 0;

};

class DX12DepthStencil : public IDepthStencil
{

public:
	OpaqueData<8> GetData() const override { return mDesc; }
	D3D12_CPU_DESCRIPTOR_HANDLE mDesc{};

};

class DX12RenderTarget : public IRenderTarget
{
public:
	DX12RenderTarget(RenderTargetDesc const& desc, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const& dxDesc);
	~DX12RenderTarget();
	OpaqueData<8> GetRTData() override { return DescriptorHdl; }
	OpaqueData<8> GetData() const override { return DescriptorHdl; }
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHdl{};
	ComPtr<ID3D12Resource> mResource;

};


}
