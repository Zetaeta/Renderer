#include "DX12Texture.h"
#include "render/dx11/SharedD3D11.h"
#include "DX12UploadBuffer.h"
#include "DX12RHI.h"
#include <d3dx12.h>
#include "DX12Allocator.h"
#include "DX12DescriptorAllocator.h"

namespace rnd::dx12
{

DX12Texture::DX12Texture(DeviceTextureDesc const& desc, TextureData data, UploadTools& upload)
:IDeviceTexture(desc)
{
	u64 offset = CreateResource(&upload);

//	memcpy(writeAddress, data, desc.GetSize());
//	UpdateSubresources()

	D3D12_SUBRESOURCE_DATA srcData{};
	srcData.pData = data;
	srcData.RowPitch = desc.GetRowPitch();
	UpdateSubresources<1>(upload.UploadCmdList, mResource.Get(), upload.Buffer.GetCurrentBuffer(), offset,
		0u, desc.NumMips == 0 ? 1u : desc.NumMips, &srcData); // TODO mips
	TransitionState(upload.UploadCmdList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//CD3DX12_TEXTURE_COPY_LOCATION dest(mResource.Get(), 0);
	//CD3DX12_TEXTURE_COPY_LOCATION src(upload.OutUploadResource.Get(), 0);
	//upload.UploadCmdList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
}

DX12Texture::DX12Texture(DeviceTextureDesc const& desc)
:IDeviceTexture(desc)
{
	CreateResource(nullptr);
}

u64 DX12Texture::CreateResource(UploadTools* upload)
{
	auto resourceDesc = MakeResourceDesc();
	auto& rhi = GetRHI();
	mResourceSize = rhi.Device()->GetResourceAllocationInfo(0, 1, &resourceDesc).SizeInBytes;
	bool useClearVal = Desc.Flags & (TF_RenderTarget | TF_DEPTH);
	D3D12_CLEAR_VALUE clearVal;
	clearVal.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::RenderTarget);
	Zero(clearVal.Color);
	mResource = rhi.GetAllocator(EResourceType::Texture)->Allocate(resourceDesc, upload ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON, useClearVal ? &clearVal : nullptr);

	if (!!(Desc.Flags & TF_SRV))
	{
		mSrvDesc = rhi.GetDescriptorAllocator(EDescriptorType::SRV)->Allocate(EDescriptorType::SRV);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::SRV);
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = Desc.NumMips;
		rhi.Device()->CreateShaderResourceView(mResource.Get(), &srvDesc, mSrvDesc);
	}

	if (upload)
	{
		u64 uploadSize = GetRequiredIntermediateSize(mResource.Get(), 0, Desc.NumMips); // TODO mips
		auto alloc = upload->Buffer.Reserve(uploadSize, Desc.SampleCount > 1 ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
																			: D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		return alloc.Offset;
	}
	return 0;
}

D3D12_RESOURCE_DESC DX12Texture::MakeResourceDesc()
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = Desc.Width;
	resourceDesc.Height = Desc.Height;
	resourceDesc.DepthOrArraySize = Desc.ArraySize;
	resourceDesc.MipLevels = Desc.NumMips;
	resourceDesc.SampleDesc.Count = Desc.SampleCount;
	resourceDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::Resource);
	return resourceDesc;
}

DXGI_FORMAT DX12Texture::GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context /* = EDxgiFormatContext::Resource */)
{
	return dx11::GetDxgiFormat(textureFormat, context);
}

void* DX12Texture::GetTextureHandle() const
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* DX12Texture::GetData() const
{
	throw std::logic_error("The method or operation is not implemented.");
}

rnd::IRenderTarget::Ref DX12Texture::GetRT()
{
	throw std::logic_error("The method or operation is not implemented.");
}

rnd::IDepthStencil::Ref DX12Texture::GetDS()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void DX12Texture::CreateSRV()
{
	throw std::logic_error("The method or operation is not implemented.");
}

rnd::MappedResource DX12Texture::Map(u32 subResource, ECpuAccessFlags flags)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void DX12Texture::Unmap(u32 subResource)
{
	throw std::logic_error("The method or operation is not implemented.");
}

CopyableMemory<8> DX12Texture::GetShaderResource(ShaderResourceId id /*= 0*/)
{
	return mSrvDesc.ptr;
}

void* DX12Texture::GetUAV(UavId id /*= */)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void DX12Texture::TransitionState(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), fromState, toState);
}

}
