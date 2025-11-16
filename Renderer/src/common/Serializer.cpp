#include "Serializer.h"
#include "core/ClassTypeInfo.h"
#include "core/PointerTypeInfo.h"

void Serializer::SerializeString(String& str)
{
	// Default implementation: Serialize length + bytes 
	int length = NumCast<int>(str.size());
	*this << length;
	if (IsLoading()) // not strictly necessary
	{
		str.resize(length);
	}

	SerializeMemory(Span<uint8>{reinterpret_cast<uint8*>(str.data()), str.size()});
}

static bool ShouldSerializeType(TypeInfo const& type)
{
	if (type.IsClass())
	{
		return !static_cast<ClassTypeInfo const&>(type).HasAnyClassFlags(EClassFlags::Transient);
	}
	else if (type.GetTypeCategory() == ETypeCategory::POINTER)
	{
		return ShouldSerializeType(static_cast<PointerTypeInfo const&>(type).GetTargetType());
	}
	else
	{
		return true;
	}
}

bool Serializer::ShouldSerialize(const PropertyInfo& prop)
{
	return ShouldSerializeType(prop.GetType());
}

void Serializer::SerializeGeneralClass(ClassValuePtr value)
{
	EnterStringMap();
	ClassValuePtr actual = value.Downcast();
	if (IsLoading())
	{
		std::string_view key;
		while (ReadNextMapKey(key))
		{
			Name keyName(key);
			if (PropertyInfo const* prop = &actual.GetType().FindPropertyChecked(keyName); ZE_ENSURE(prop))
			{
				if (ShouldSerialize(*prop))
				{
					prop->GetType().Serialize(*this, prop->Access(actual)); 
				}
				else
				{
					ZE_ASSERT(false);
				}
			}
		}
	}
	else
	{
		actual.GetType().ForEachProperty([this, actual](PropertyInfo const& prop)
		{
			auto child = prop.Access(actual);
			if (ShouldSerialize(prop))
			{
				SerializeMapKey(prop.GetNameStr());
				prop.GetType().Serialize(*this, prop.Access(actual));
			}
		});
	}
	LeaveStringMap();
}
