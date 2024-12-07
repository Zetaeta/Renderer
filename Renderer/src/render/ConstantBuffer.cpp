#include "ConstantBuffer.h"

namespace rnd
{

DataLayout MakeCBLayout(ClassTypeInfo const& classType, size_t alignment)
{
	Vector<DataLayoutEntry> entries;
	classType.ForEachProperty([&entries](PropertyInfo const& prop) {
		entries.emplace_back(DataLayoutEntry{ &prop.GetType(), prop.GetName(), static_cast<size_t>(prop.GetOffset()) });
	});
	return DataLayout(alignment, std::move(entries));
}

void ConstantBufferData::SetLayout(DataLayout::Ref layout)
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
		ZE_ASSERT(false, "no layout");
		return;
	}

	for (DataLayoutEntry const& entry : Layout->Entries)
	{
		if (ConstReflectedValue sourceEntry = source.Get(entry.mName))
		{
			ReflectedValue val = CBAccessor {this, &entry}.Access();
			ZE_ASSERT(val.GetType() == sourceEntry.GetType());
			val.GetType().Copy(sourceEntry, val);
		}
	}
}

}
