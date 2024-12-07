#pragma once
#include "DeviceResource.h"
#include "DeviceTexture.h"

namespace rnd { class RenderPass; }

namespace rnd
{

enum class ERGResourceBevhiour : u8
{
	FlipFlop,
	FixedSRV
};

struct RGResourceHandle
{
	u32 Hdl;
	ERGResourceBevhiour Type;
};

class IRGResource
{
public:
	virtual IRenderTarget::Ref GetRenderTarget(RGResourceHandle instanceHandle)
	{
		ZE_ASSERT(false);
		return {};
	}

	virtual ResourceView GetResourceView(RGResourceHandle instanceHandle)
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

class RGBuilder
{
public:
	RGResourceHandle AddFixedSRV(ResourceView view)
	{
		mFixedSRVs.push_back(RGFixedSRV{view});
		return RGResourceHandle{NumCast<u32>(mFixedSRVs.size() - 1), ERGResourceBevhiour::FixedSRV};
	}
	RGResourceHandle MakeFlipFlop(DeviceTextureRef tex1, DeviceTextureRef tex2);

	IRGResource* operator[](RGResourceHandle handle)
	{
		switch (handle.Type)
		{
		case ERGResourceBevhiour::FlipFlop:
			return &mFlipFlops[handle.Hdl];
		case ERGResourceBevhiour::FixedSRV:
			return &mFixedSRVs[handle.Hdl];
		default:
			ZE_ASSERT(false);
			return nullptr;
		}
	}

	void Reset();
private:
	Vector<RGFlipFlop> mFlipFlops;
	Vector<RGFixedSRV> mFixedSRVs;
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
			ResolvedViews[i] = builder[Handles[i]]->GetResourceView(Handles[i]);
		}
	}

	size_t Count() { return Handles.size(); }
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

}
