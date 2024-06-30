#pragma once
#include "SharedD3D11.h"
#include "DX11Ctx.h"
#include <core/TypeInfoUtils.h>
#include <render/ConstantBuffer.h>

struct PerFrameVertexData;

namespace rnd
{
namespace dx11
{

class DX11ConstantBuffer
{
public:
	DX11ConstantBuffer() {}

	template <typename T>
	DX11ConstantBuffer(DX11Ctx* ctx, T const& initialData, u32 size = sizeof(T))
		: mCtx(ctx)
	{
		CBLayout::Ref layout = nullptr;
		if constexpr (HasClassTypeInfo<std::remove_cvref_t<T>>)
		{
			layout = GetLayout<T>();
		}
		else
		{
			static_assert(!std::is_same_v<std::remove_cvref_t<T>, PerFrameVertexData>);
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

	template<typename T>
	void WriteData(const T& val)
	{
		mData.SetData(val);
		Update();
	}

	ID3D11Buffer** GetAddressOf()
	{
		return mDeviceBuffer.GetAddressOf();
	}

	ConstantBufferData& GetCBData() { return mData; }

	DX11ConstantBuffer(DX11Ctx* ctx, u32 size, CBLayout::Ref layout = nullptr);

	ID3D11Buffer* GetDeviceBuffer() { return mDeviceBuffer.Get(); }
	ID3D11Buffer* Get() { return mDeviceBuffer.Get(); }

	void Update();
	//void Bind(u32 idx);

private:
	void CreateDeviceResource();
	ConstantBufferData mData;
	DX11Ctx* mCtx = nullptr;
	ComPtr<ID3D11Buffer> mDeviceBuffer;
};

}
}
