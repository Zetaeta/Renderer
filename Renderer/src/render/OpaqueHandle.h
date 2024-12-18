#pragma once
namespace rnd
{

template<size_t Size = 8>
struct SimpleOpaqueHandle
{
	template<typename T>
	T GetOpaqueData() const
	{
		return mPrivateData.As<T>();
	}
protected:
	CopyableMemory<8> mPrivateData;
};

}
