#pragma once

#include "SharedD3D12.h"
#include "render/DeviceTexture.h"

namespace rnd { namespace dx12 { class DX12UploadBuffer; } }

namespace rnd::dx12
{

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

	static DXGI_FORMAT GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context /* = EDxgiFormatContext::Resource */);


	void* GetTextureHandle() const override;
	void* GetData() const override;
	IRenderTarget::Ref GetRT() override;
	IDepthStencil::Ref GetDS() override;
	void CreateSRV() override;
	MappedResource Map(u32 subResource, ECpuAccessFlags flags) override;
	void Unmap(u32 subResource) override;
	CopyableMemory<8> GetShaderResource(ShaderResourceId id) override;
	void* GetUAV(UavId id) override;

	void TransitionState(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState);

private:
	D3D12_RESOURCE_DESC MakeResourceDesc();
	// Returns offset in upload buffer if uploading
	u64 CreateResource(_In_opt_ UploadTools* upload);

	ComPtr<ID3D12Resource> mResource;
	D3D12_CPU_DESCRIPTOR_HANDLE mSrvDesc{};
	u64 mResourceSize = 0;

};

}
