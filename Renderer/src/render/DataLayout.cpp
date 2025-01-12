#include "DataLayout.h"

namespace rnd
{
DataLayout::DataLayout(size_t alignment, Vector<DataLayoutEntry>&& entries)
	: mAlignment(alignment), Entries(std::move(entries)), mSize(0)
{
	for (auto& entry : Entries)
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
} // namespace rnd
