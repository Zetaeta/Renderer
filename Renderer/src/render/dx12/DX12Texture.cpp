#include "DX12Texture.h"
#include "render/dx11/SharedD3D11.h"
#include "DX12UploadBuffer.h"
#include "DX12RHI.h"
#include <d3dx12.h>
#include "DX12Allocator.h"
#include "DX12DescriptorAllocator.h"
#include "render/dxcommon/DXGIUtils.h"

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

DX12Texture::DX12Texture(DeviceTextureDesc const& desc, ID3D12Resource_* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
:IDeviceTexture(desc), mResource(resource)
{
	SetResourceName(mResource, Desc.DebugName);
	RenderTargetDesc rtDesc;
	rtDesc.Width = Desc.Width;
	rtDesc.Height = Desc.Height;
	rtDesc.Format = Desc.Format;
	rtDesc.DebugName = Desc.DebugName + " RT";
	mRtvOrDsv = DX12RenderTarget(rtDesc, this, resource, rtvHandle);
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
	if (Desc.Flags & TF_DEPTH)
	{
		clearVal.Color[0] = 1.f;
	}
	mLastState = upload ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON;
	mResource = rhi.GetAllocator(EResourceType::Texture)->Allocate(resourceDesc, mLastState, useClearVal ? &clearVal : nullptr);
	SetResourceName(mResource, Desc.DebugName);

	if (!!(Desc.Flags & TF_SRV))
	{
		mSrvDesc = rhi.GetDescriptorAllocator(EDescriptorType::SRV)->Allocate(EDescriptorType::SRV);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		if (Desc.ResourceType == EResourceType::Texture2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = Desc.NumMips <= 0 ? 1 : Desc.NumMips;
		}
		else
		{
			ZE_ASSERT(Desc.ResourceType == EResourceType::TextureCube);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = Desc.NumMips <= 0 ? 1 : Desc.NumMips;
		}
		srvDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::SRV);
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		rhi.Device()->CreateShaderResourceView(mResource.Get(), &srvDesc, mSrvDesc);
	}
	if (!!(Desc.Flags & TF_StencilSRV))
	{
		mStencilSrvDesc = rhi.GetDescriptorAllocator(EDescriptorType::SRV)->Allocate(EDescriptorType::SRV);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		if (Desc.ResourceType == EResourceType::Texture2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = Desc.NumMips <= 0 ? 1 : Desc.NumMips;
			srvDesc.Texture2D.PlaneSlice = 1;
		}
		else
		{
			ZE_ASSERT(Desc.ResourceType == EResourceType::TextureCube);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = Desc.NumMips <= 0 ? 1 : Desc.NumMips;
		}
		srvDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::StencilSRV);
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		rhi.Device()->CreateShaderResourceView(mResource.Get(), &srvDesc, mStencilSrvDesc);
	}
	if (!!(Desc.Flags & TF_UAV))
	{
		mUavDesc = rhi.GetDescriptorAllocator(EDescriptorType::UAV)->Allocate(EDescriptorType::UAV);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		if (Desc.ResourceType == EResourceType::Texture2D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D = {};
			uavDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::UAV);
			rhi.Device()->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, mUavDesc);
		}
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
		if (Desc.ResourceType == EResourceType::Texture2D)
		{
			if (Desc.SampleCount == 1)
			{
				dxDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				dxDesc.Texture2D = {};
			}
			else
			{
				dxDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			}
		}
		else if (Desc.ResourceType == EResourceType::TextureCube)
		{
			dxDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			dxDesc.Texture2DArray.ArraySize = 6;
			dxDesc.Texture2DArray.FirstArraySlice = 0;
			dxDesc.Texture2DArray.MipSlice = 0;
			rtDesc.Dimension = ETextureDimension::TEX_CUBE;
		}
		mRtvOrDsv = DX12RenderTarget(rtDesc, this, mResource.Get(), dxDesc);
	}
	if (!!(Desc.Flags & TF_DEPTH))
	{
		DepthStencilDesc dsDesc;
		dsDesc.DebugName = Desc.DebugName + " RT";
		dsDesc.Format = Desc.Format;
		dsDesc.Dimension = ETextureDimension::TEX_2D;
		dsDesc.Width = Desc.Width;
		dsDesc.Height = Desc.Height;
		D3D12_DEPTH_STENCIL_VIEW_DESC dxDesc {};
		dxDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::DepthStencil);
		if (Desc.ResourceType == EResourceType::Texture2D)
		{
			if (Desc.SampleCount == 1)
			{
				dxDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dxDesc.Texture2D = {};
			}
			else
			{
				dxDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
				dxDesc.Texture2DMS = {};
			}
		}
		else if (Desc.ResourceType == EResourceType::TextureCube)
		{
			dxDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dxDesc.Texture2DArray.ArraySize = 6;
			dxDesc.Texture2DArray.FirstArraySlice = 0;
			dxDesc.Texture2DArray.MipSlice = 0;
			dsDesc.Dimension = ETextureDimension::TEX_CUBE;
		}
		mRtvOrDsv = DX12DepthStencil(dsDesc, this, mResource.Get(), dxDesc);
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
	if (Desc.ResourceType == EResourceType::TextureCube)
	{
		resourceDesc.DepthOrArraySize *= 6;
	}
	resourceDesc.MipLevels = Desc.NumMips;
	resourceDesc.SampleDesc.Count = Desc.SampleCount;
	resourceDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::Resource);
	if (Desc.Flags & TF_RenderTarget)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if (Desc.Flags & TF_DEPTH)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	if (Desc.Flags & TF_UAV)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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
	return mSrvDesc;
}

rnd::IRenderTarget::Ref DX12Texture::GetRT()
{
	if (std::holds_alternative<DX12RenderTarget>(mRtvOrDsv))
	{
		return std::shared_ptr<IRenderTarget>(shared_from_this(), &std::get<DX12RenderTarget>(mRtvOrDsv));
	}
	return nullptr;
}

rnd::IDepthStencil::Ref DX12Texture::GetDS()
{
	if (std::holds_alternative<DX12DepthStencil>(mRtvOrDsv))
	{
		return std::shared_ptr<IDepthStencil>(shared_from_this(), &std::get<DX12DepthStencil>(mRtvOrDsv));
	}
	return nullptr;
}

void DX12Texture::CreateSRV(u32 srvIndex)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* srvDescriptor = nullptr;
	if (srvIndex == 0)
	{
		srvDescriptor = &mSrvDesc;
	}
	else
	{
		if (mAdditionalSRVs.size() < srvIndex)
		{
			mAdditionalSRVs.resize(srvIndex);
		}
		srvDescriptor = &mAdditionalSRVs[srvIndex - 1];
	}
	if (srvDescriptor->ptr != 0)
	{
		return;
	}
	auto& rhi = GetRHI();
	*srvDescriptor = rhi.GetDescriptorAllocator(EDescriptorType::SRV)->Allocate(EDescriptorType::SRV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	if (Desc.ResourceType == EResourceType::Texture2D)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = -1;
		srvDesc.Texture2D.MostDetailedMip = srvIndex;
	}
	else
	{
		ZE_ASSERT(Desc.ResourceType == EResourceType::TextureCube);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = -1;//Desc.NumMips <= 0 ? 1 : Desc.NumMips;
	}
	srvDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::SRV);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	rhi.Device()->CreateShaderResourceView(mResource.Get(), &srvDesc, *srvDescriptor);
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
	if (id == SRV_StencilBuffer)
	{
		ZE_ASSERT(IsValid(mStencilSrvDesc));
		return mStencilSrvDesc;
	}
	ZE_ASSERT(Desc.Flags & TF_SRV);
	if ((id & u32(-1)) == 0)
	{
		return mSrvDesc;
	}
	else
	{
		return mAdditionalSRVs[id - 1];
	}
}

OpaqueData<8> DX12Texture::GetUAV(UavId id /*= */)
{
	ZE_ASSERT(Desc.Flags & TF_UAV);
	if (id > 0)
	{
		return mAdditionalUAVs[id - 1];
	}
	return mUavDesc;
}

void DX12Texture::TransitionState(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState)
{
	//if (mLastState != D3D12_RESOURCE_STATE_COMMON)
	{
//		RLOG(LogGlobal, Verbose, "Transitioning %s from %d to %d", Desc.DebugName.c_str(), fromState, toState);
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), fromState, toState);
		cmdList->ResourceBarrier(1, &barrier);
	}
	mLastState = toState;
}

void DX12Texture::TransitionTo(ID3D12GraphicsCommandList_* cmdList, D3D12_RESOURCE_STATES toState)
{
	if (mLastState != toState)
	{
		TransitionState(cmdList, mLastState, toState);
	}
}

DX12Texture::~DX12Texture()
{
	auto& rhi = GetRHI();
	rhi.FreeDescriptor(EDescriptorType::SRV, mSrvDesc);
//	rhi.FreeDescriptor(EDescriptorType::SRV, mStencilSrvDesc);
	for (auto desc : mAdditionalSRVs)
	{
		rhi.FreeDescriptor(EDescriptorType::SRV, desc);
	}
	rhi.FreeDescriptor(EDescriptorType::UAV, mUavDesc);
	for (auto desc : mAdditionalUAVs)
	{
		rhi.FreeDescriptor(EDescriptorType::UAV, desc);
	}
	rhi.DeferredRelease(mResource);

}

UavId DX12Texture::CreateUAV(u32 subresourceIdx)
{
	auto&							 rhi = GetRHI();
	D3D12_CPU_DESCRIPTOR_HANDLE* uavDescriptor = nullptr;
	if (subresourceIdx > 0)
	{
		if (mAdditionalUAVs.size() < subresourceIdx)
		{
			mAdditionalUAVs.resize(subresourceIdx);
		}
		uavDescriptor = &mAdditionalUAVs[subresourceIdx - 1];
	}
	else
	{
		uavDescriptor = &mUavDesc;
	}
	*uavDescriptor = rhi.GetDescriptorAllocator(EDescriptorType::UAV)->Allocate(EDescriptorType::UAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	if (Desc.ResourceType == EResourceType::Texture2D)
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D = {};
		uavDesc.Texture2D.MipSlice = subresourceIdx;
		uavDesc.Format = GetDxgiFormat(Desc.Format, ETextureFormatContext::UAV);
		rhi.Device()->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, *uavDescriptor);
	}
	return subresourceIdx;
}

DX12RenderTarget::~DX12RenderTarget()
{
	if (!BaseTex)
	{
		return;
	}
	for (int i = 0; i < DescriptorHdl.size(); ++i)
	{
		if (IsValid(DescriptorHdl[i]))
		{
			GetRHI().FreeDescriptor(EDescriptorType::RTV, DescriptorHdl[i]);
		}
	}
}

DX12RenderTarget::DX12RenderTarget(RenderTargetDesc const& desc, DX12Texture* tex, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const& dxDesc)
:IRenderTarget(desc), BaseTex(tex)
{
	auto& rhi = GetRHI();
	if (dxDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY)
	{
		auto subDesc = dxDesc;
		for (u32 i = dxDesc.Texture2DArray.FirstArraySlice; i < dxDesc.Texture2DArray.ArraySize;++i)
		{
			subDesc.Texture2DArray.FirstArraySlice = i;
			DescriptorHdl[i] = rhi.GetDescriptorAllocator(EDescriptorType::RTV)->Allocate(EDescriptorType::RTV);
			rhi.Device()->CreateRenderTargetView(resource, &subDesc, DescriptorHdl[i]);
		}
	}
	else
	{
		DescriptorHdl[0] = rhi.GetDescriptorAllocator(EDescriptorType::RTV)->Allocate(EDescriptorType::RTV);
		rhi.Device()->CreateRenderTargetView(resource, &dxDesc, DescriptorHdl[0]);

	}
}

DX12RenderTarget::DX12RenderTarget(RenderTargetDesc const& desc, DX12Texture* tex, ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE existingRtvDesc)
:IRenderTarget(desc), BaseTex(tex)
{
	DescriptorHdl[0] = existingRtvDesc;
}

DX12RenderTarget::DX12RenderTarget(DX12RenderTarget&& other)
:IRenderTarget(other.Desc), BaseTex(other.BaseTex),DescriptorHdl(other.DescriptorHdl)
{
	other.BaseTex = nullptr;
	other.DescriptorHdl = {};
}

DX12RenderTarget& DX12RenderTarget::operator=(DX12RenderTarget&& other)
{
	Desc = other.Desc;
	BaseTex = other.BaseTex;
	DescriptorHdl = other.DescriptorHdl;
	other.BaseTex = nullptr;
	other.DescriptorHdl = {};
	return *this;
}

DX12DepthStencil::DX12DepthStencil(DepthStencilDesc const& desc, DX12Texture* tex, ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const& dxDesc)
:IDepthStencil(desc), BaseTex(tex)
{
	auto& rhi = GetRHI();
	if (dxDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2DARRAY)
	{
		auto subDesc = dxDesc;
		subDesc.Texture2DArray.ArraySize = 1;
		for (u32 i = dxDesc.Texture2DArray.FirstArraySlice; i < dxDesc.Texture2DArray.ArraySize;++i)
		{
			subDesc.Texture2DArray.FirstArraySlice = i;
			mDesc[i] = rhi.GetDescriptorAllocator(EDescriptorType::DSV)->Allocate(EDescriptorType::DSV);
			rhi.Device()->CreateDepthStencilView(resource, &subDesc, mDesc[i]);
		}
	}
	else
	{
		mDesc[0] = rhi.GetDescriptorAllocator(EDescriptorType::DSV)->Allocate(EDescriptorType::DSV);
		rhi.Device()->CreateDepthStencilView(resource, &dxDesc, mDesc[0]);
	}
}

DX12DepthStencil::DX12DepthStencil(DX12DepthStencil&& other)
	:IDepthStencil(other.Desc), BaseTex(other.BaseTex), mDesc(other.mDesc)
{
	other.BaseTex = nullptr;
	other.mDesc = {};
}

DX12DepthStencil::~DX12DepthStencil()
{
	if (!BaseTex)
	{
		return;
	}
	for (u32 i = 0; i < mDesc.size(); ++i)
	{
		if (IsValid(mDesc[i]))
		{
			GetRHI().FreeDescriptor(EDescriptorType::DSV, mDesc[i]);
		}
	}
}

DX12DepthStencil& DX12DepthStencil::operator=(DX12DepthStencil&& other)
{
	Desc = other.Desc;
	BaseTex = other.BaseTex;
	mDesc = other.mDesc;
	other.BaseTex = nullptr;
	other.mDesc = {};
	return *this;
}

}
