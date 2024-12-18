#pragma once
#include "DeviceResource.h"
#include "DeviceTexture.h"

namespace rnd
{
class IRenderDevice;
class RenderPass;
enum class ERGResourceBevhiour : u8
{
	Invalid,
	FlipFlop,
	Texture,
	FixedSRV
};

struct RGResourceHandle
{
	RGResourceHandle() = default;
	RGResourceHandle(std::nullptr_t null) {};
	RGResourceHandle(u32 hdl, ERGResourceBevhiour type, u32 subresource = 0)
	:Hdl(hdl), Type(type), Subresource(subresource) { }
	u32 Hdl = (u32) -1;
	u32 Subresource : 24 = 0;
	ERGResourceBevhiour Type = ERGResourceBevhiour::Invalid;
};

class IRGResource
{
public:
	virtual IRenderTarget::Ref GetRenderTarget(RGResourceHandle instanceHandle)
	{
		ZE_ASSERT(false);
		return {};
	}
	virtual IDepthStencil::Ref GetDSV(RGResourceHandle instanceHandle)
	{
		ZE_ASSERT(false);
		return {};
	}

	virtual ResourceView GetResourceView(RGResourceHandle instanceHandle)
	{
		ZE_ASSERT(false);
		return {};
	}

	virtual UnorderedAccessView GetUAV(RGResourceHandle instanceHandle)
	{
		ZE_ASSERT(false);
		return {};
	}
};

class RGFlipFlop final : public IRGResource
{
public:
	RGFlipFlop(DeviceTextureRef tex1, DeviceTextureRef tex2)
	{
		Textures[0] = tex1;
		Textures[1] = tex2;
	}
	IRenderTarget::Ref GetRenderTarget(RGResourceHandle instanceHandle);
	ResourceView	   GetResourceView(RGResourceHandle instanceHandle);
	bool			   IsOdd() const
	{
		return CurrentRead == 1;
	}

	void Reset()
	{
		CurrentRead = 0;
	}

private:
	DeviceTextureRef Textures[2];
	u8 CurrentRead = 0;
};

class RGFixedSRV final : public IRGResource
{
public:
	RGFixedSRV(ResourceView view)
	:View(view) {}

	ResourceView GetResourceView(RGResourceHandle instanceHdl)
	{
		return View;
	}
	ResourceView View;
};

class RGTexture : public IRGResource
{
public:
	RGTexture(DeviceTextureRef&& texture)
	:mTexture(texture) { }

	IRenderTarget::Ref GetRenderTarget(RGResourceHandle instanceHandle);
	IDepthStencil::Ref GetDSV(RGResourceHandle instanceHandle);
	ResourceView GetResourceView(RGResourceHandle instanceHandle);
	UnorderedAccessView GetUAV(RGResourceHandle instanceHandle);
private:
	DeviceTextureRef mTexture;
};

class RGBuilder
{
public:
	RGBuilder(IRenderDevice* device)
	:mDevice(device) { }
	RGResourceHandle AddFixedSRV(ResourceView view);
	RGResourceHandle MakeFlipFlop(DeviceTextureRef tex1, DeviceTextureRef tex2);

	RGResourceHandle MakeTexture2D(DeviceTextureDesc const& desc);

	IRGResource* operator[](RGResourceHandle handle)
	{
		switch (handle.Type)
		{
		case ERGResourceBevhiour::FlipFlop:
			return &mFlipFlops[handle.Hdl];
		case ERGResourceBevhiour::FixedSRV:
			return &mFixedSRVs[handle.Hdl];
		case ERGResourceBevhiour::Texture:
			return &mTextures[handle.Hdl];
		case ERGResourceBevhiour::Invalid:
			return nullptr;
		default:
			ZE_ASSERT(false);
			return nullptr;
		}
	}

	ResourceView GetSRV(RGResourceHandle handle)
	{
		return this->operator[](handle)->GetResourceView(handle);
	}

	UnorderedAccessView GetUAV(RGResourceHandle handle)
	{
		return this->operator[](handle)->GetUAV(handle);
	}

	void Reset();
private:
	Vector<RGFlipFlop> mFlipFlops;
	Vector<RGFixedSRV> mFixedSRVs;
	Vector<RGTexture> mTextures;
	IRenderDevice* mDevice = nullptr;
};

struct RGResourceRef
{
	RGResourceHandle Handle;
	void Write(RGBuilder& builder, RenderPass* pass) {}
	void Read(RGBuilder& builder, RenderPass* pass) {}
	void ReadWrite(RGBuilder& builder, RenderPass* pass) {}
};

struct RGResourceViewRef : RGResourceRef
{
	ResourceView ResolvedView;
	void Resolve(RGBuilder& builder)
	{
		ResolvedView = builder[Handle]->GetResourceView(Handle);
	}
};

struct RGShaderResources
{
	RGShaderResources(Vector<RGResourceHandle>&& handles)
	:Handles(std::move(handles)), ResolvedViews(Handles.size()) {}

	Vector<RGResourceHandle> Handles;
	Vector<ResourceView> ResolvedViews;
	void Resolve(RGBuilder& builder)
	{
		for (int i = 0; i < Handles.size(); ++i)
		{
			if (auto resource = builder[Handles[i]])
			{
				ResolvedViews[i] = resource->GetResourceView(Handles[i]);
			}
			else
			{
				ResolvedViews[i] = {};
			}
		}
	}

	size_t Count() { return Handles.size(); }
};

struct RGUnorderedAccessViews
{
	RGUnorderedAccessViews(Vector<RGResourceHandle>&& handles)
	:Handles(std::move(handles)), ResolvedViews(Handles.size()) {}

	Vector<RGResourceHandle> Handles;
	Vector<UnorderedAccessView> ResolvedViews;
	void Resolve(RGBuilder& builder)
	{
		for (int i = 0; i < Handles.size(); ++i)
		{
			ResolvedViews[i] = builder[Handles[i]]->GetUAV(Handles[i]);
		}
	}

	u32 Count() { return NumCast<u32>(Handles.size()); }
};

struct RGRenderTargetRef
{
	RGRenderTargetRef(RGResourceHandle hdl)
	:Handle(hdl) {}
	RGResourceHandle Handle;
	IRenderTarget::Ref ResolvedRT = nullptr;
	void Resolve(RGBuilder& builder)
	{
		ResolvedRT = builder[Handle]->GetRenderTarget(Handle);
	}
};

struct RGDepthStencilRef
{
	RGDepthStencilRef(RGResourceHandle hdl)
	:Handle(hdl) {}
	RGResourceHandle Handle;
	IDepthStencil::Ref ResolvedDS = nullptr;
	void Resolve(RGBuilder& builder)
	{
		ResolvedDS = builder[Handle]->GetDSV(Handle);
	}
};

}
