#include "ConstantBuffer.h"

namespace rnd
{

CBLayout::CBLayout(size_t alignment, Vector<CBLEntry>&& entries)
	: mAlignment(alignment), Entries(std::move(entries))
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

CBLayout MakeCBLayout(ClassTypeInfo const& classType, size_t alignment)
{
	Vector<CBLEntry> entries;
	classType.ForEachProperty([&entries](PropertyInfo const& prop) {
		entries.emplace_back(CBLEntry{ &prop.GetType(), prop.GetName(), static_cast<size_t>(prop.GetOffset()) });
	});
	return CBLayout(alignment, std::move(entries));
}

void ConstantBufferData::SetLayout(CBLayout::Ref layout)
{
	if (layout != nullptr)
	{
		if (layout->GetSize() > DataSize)
		{
			Resize(layout->GetSize());
		}
	}
	Layout = layout;
}

void ConstantBufferData::FillFromSource(const CBDataSource& source)
{
	if (!Layout)
	{
		RASSERT(false, "no layout");
		return;
	}

	for (CBLEntry const& entry : Layout->Entries)
	{
		if (ConstReflectedValue sourceEntry = source.Get(entry.mName))
		{
			ReflectedValue val = CBAccessor {this, &entry}.Access();
			RASSERT(val.GetType() == sourceEntry.GetType());
			val.GetType().Copy(sourceEntry, val);
		}
	}
}

}
