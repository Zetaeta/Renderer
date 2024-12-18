#pragma once

#include "core/Maths.h"
#include "DX11ConstantBuffer.h"
#include <utility>
#include "core/memory/GrowingImmobileObjectPool.h"

using byte = unsigned char;

namespace rnd
{
namespace dx11
{

using SimpleCB = ID3D11Buffer*;

class DX11CBPool : public ICBPool
{
public:
	DX11CBPool(ID3D11Device* pDevice, IRenderDeviceCtx* pContext)
		: mDevice(pDevice), mContext(pContext) {}

	//struct Initializer
	//{
	//	Initializer(u32 size)
	//		: Size(size) {}
	//	u32 Size;
	//	void operator()(SimpleCB& cb) const
	//	{
	//		
	//	}
	//};

	constexpr static u32 SmallestPoolLog = 4;
	constexpr static u32 SmallestPoolBytes = Pow2(SmallestPoolLog); // 16

	PooledCBHandle AcquireConstantBuffer(u32 size, std::span<const byte> initialData) override
	{
		static u8 dummy[4096];

		u32 log2Size = RoundUpLog2(size);
		if (log2Size < SmallestPoolLog)
		{
			log2Size = SmallestPoolLog;
		}

		u32 index = log2Size - SmallestPoolLog;
		if (mPools.size() <= index)
		{
			mPools.resize(index + 1);
		}
			
		u32 id;
		SimpleCB* ppBuffer;
		bool isNew = mPools[index].Claim(id, ppBuffer);
		PooledCBHandle result;
		result.Id = u64(id) | (u64(index) << 32);
		if (isNew)
		{
			D3D11_BUFFER_DESC cbDesc;
			Zero(cbDesc);
			cbDesc.ByteWidth = Pow2(log2Size);
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA srData;
			Zero(srData);
			srData.pSysMem = &dummy;
			memcpy(dummy, &initialData[0], initialData.size());
			HR_ERR_CHECK(mDevice->CreateBuffer(&cbDesc, &srData, ppBuffer));
		}

		result.UserData.As<ID3D11Buffer*>() = *ppBuffer;
		if (!isNew)
		{
			mContext->UpdateConstantBuffer(result, initialData);
		}
		return result;
	}

	void ReleaseConstantBuffer(PooledCBHandle handle) override
	{
		u64 poolIdx = handle.Id >> 32;
		u32 objIdx = u32(handle.Id & bits::Mask32);
		mPools[poolIdx].Release(objIdx);
	}
private:
	Vector<GrowingImmobileObjectPool<SimpleCB>> mPools;
	ID3D11Device* mDevice;
	IRenderDeviceCtx* mContext;
};
}
}
