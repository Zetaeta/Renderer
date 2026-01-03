#pragma once
#include "core/TypeInfo.h"
#include "core/Types.h"

namespace rnd
{

struct DataLayoutEntry
{
	TypeInfo const* mType;
	HashString mName;
	size_t mOffset = 0;
};

template<typename T>
DataLayoutEntry Entry(HashString name)
{
	return DataLayoutEntry { &GetTypeInfo<T>(), name };
}

struct DataLayout
{
	DataLayout()
		: mAlignment(1), mSize(0) {}
	DataLayout(size_t alignment, Vector<DataLayoutEntry>&& entries);

	size_t					mAlignment;
	Vector<DataLayoutEntry> Entries;
	size_t					mSize;

	size_t GetSize() const
	{
		return mSize;
	}

	using Ref = DataLayout*;
};
} // namespace rnd
