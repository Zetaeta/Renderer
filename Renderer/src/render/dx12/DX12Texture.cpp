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
	SetResourceName(mResource, Desc.DebugName);

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
	if (!!(Desc.Flags & TF_RenderTarget))
	{
		RenderTargetDesc rtDesc;
		rtDesc.DebugName = Desc.DebugName + " RT";
		rtDesc.Format = Desc.Format;
		rtDesc.Dimension = ETextureDimension::TEX_2D;
		rtDesc.Width = Desc.Width;
		rtDesc.Height = Desc.Height;
		D3D12_RENDER_TARGET_VIEW_DESC dxDesc {};
		dxDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::RenderTarget);
		if (Desc.SampleCount > 1)
		{
			dxDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			dxDesc.Texture2D = {};
		}
		else
		{
			dxDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		}
		mRtv = std::make_shared<DX12RenderTarget>(rtDesc, mResource.Get(), dxDesc);
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
	if (Desc.Flags & TF_RenderTarget)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	return resourceDesc;
}

//DXGI_FORMAT DX12Texture::GetDxgiFormat(ETextureFormat textureFormat, ETextureFormatContext context /* = EDxgiFormatContext::Resource */)
//{
//	return GetDxgiFormat(textureFormat, context);
//}

OpaqueData<8> DX12Texture::GetTextureHandle() const
{
	return mResource.Get();
}

OpaqueData<8> DX12Texture::GetData() const
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

OpaqueData<8> DX12Texture::GetUAV(UavId id /*= */)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void DX12Texture::TransitionState(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), fromState, toState);
	cmdList->ResourceBarrier(1, &barrier);
	mLastState = toState;
}

void DX12Texture::TransitionTo(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES toState)
{
	TransitionState(cmdList, mLastState, toState);
}

DX12Texture::~DX12Texture()
{
	auto& rhi = GetRHI();
	if (IsValid(mSrvDesc))
	{
		rhi.FreeDescriptor(EDescriptorType::SRV, mSrvDesc);
	}

}

DX12RenderTarget::~DX12RenderTarget()
{
	if (IsValid(DescriptorHdl))
	{
		GetRHI().FreeDescriptor(EDescriptorType::SRV, DescriptorHdl);
	}
}

DX12RenderTarget::DX12RenderTarget(RenderTargetDesc const& desc, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const& dxDesc)
:IRenderTarget(desc), mResource(resource)
{
	auto& rhi = GetRHI();
	DescriptorHdl = rhi.GetDescriptorAllocator(EDescriptorType::RTV)->Allocate(EDescriptorType::RTV);
	rhi.Device()->CreateRenderTargetView(resource, &dxDesc, DescriptorHdl);
}

}
