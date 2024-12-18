#pragma once
#include "SharedD3D11.h"
#include "DX11Ctx.h"
#include <core/TypeInfoUtils.h>
#include <render/ConstantBuffer.h>

namespace rnd
{
namespace dx11
{

class DX11ConstantBuffer : public IConstantBuffer
{
public:
	DX11ConstantBuffer() {}

	template <typename T>
	DX11ConstantBuffer(DX11Ctx* ctx, T const& initialData, u32 size = sizeof(T))
		: mCtx(ctx)
	{
		DataLayout::Ref layout = nullptr;
		if constexpr (HasClassTypeInfo<std::remove_cvref_t<T>>)
		{
			layout = GetLayout<T>();
		}

		mData = ConstantBufferData(size, layout);
		memcpy(mData.GetData(), &initialData, sizeof(initialData));
		CreateDeviceResource();
	}

	template<typename T>
	static DX11ConstantBuffer CreateWithLayout(DX11Ctx* ctx)
		requires(HasClassTypeInfo<std::remove_cvref_t<T>>)
	{
		return DX11ConstantBuffer(ctx, T{});
	}

	template<typename T>
	static DX11ConstantBuffer Create(DX11Ctx* ctx) {
		return DX11ConstantBuffer(ctx, T{});
	}

	ID3D11Buffer** GetAddressOf()
	{
		return mDeviceBuffer.GetAddressOf();
	}

	ConstantBufferData& GetCBData() { return mData; }

	DX11ConstantBuffer(DX11Ctx* ctx, u32 size, DataLayout::Ref layout = nullptr);

	ID3D11Buffer* GetDeviceBuffer() { return mDeviceBuffer.Get(); }
	ID3D11Buffer* Get() { return mDeviceBuffer.Get(); }

	void SetLayout(DataLayout::Ref layout) override
	{
		u64 currentSize = mData.GetSize();
		if (layout->GetSize() > mData.GetSize())
		{
			mData = ConstantBufferData(currentSize * 2, layout);
			CreateDeviceResource();
		}
		else
		{
			mData.SetLayout(layout);
		}
	}

	void Resize(size_t size);

	static size_t GetActualSize(size_t inSize)
	{
		size_t floorSize = (inSize / 16) * 16;
		if (floorSize == inSize)
		{
			return inSize;
		}
		return floorSize + 16;
	}

	void Update();
	//void Bind(u32 idx);

private:
	void CreateDeviceResource();
	DX11Ctx* mCtx = nullptr;
	ComPtr<ID3D11Buffer> mDeviceBuffer;
};

}
}
