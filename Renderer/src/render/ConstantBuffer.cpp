#include "ConstantBuffer.h"

namespace rnd
{

CBLayout::CBLayout(size_t alignment, Vector<CBLEntry>&& entries)
	: mAlignment(alignment), mEntries(std::move(entries))
{
	for (auto& entry : mEntries)
	{
		auto currAlign = mSize % mAlignment;
		auto nextSize = entry.mType->GetSize();
		if (currAlign > 0 && currAlign + nextSize > mAlignment)
		{
			mSize += mAlignment - currAlign;
		}

		entry.mOffset = mSize;
		mSize += nextSize;
	}
}

CBLayout MakeCBLayout(ClassTypeInfo const& classType, size_t alignment)
{
	Vector<CBLEntry> entries;
	classType.ForEachProperty([&entries](PropertyInfo const& prop) {
		entries.emplace_back(CBLEntry{ &prop.GetType(), prop.GetName(), static_cast<size_t>(prop.GetOffset()) });
	});
	return CBLayout(alignment, std::move(entries));
}

}
