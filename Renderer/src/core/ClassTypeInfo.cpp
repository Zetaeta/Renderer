#include "ClassTypeInfo.h"
#include "BaseObject.h"

ReflectedValue PropertyInfo::Access(ClassValuePtr const& obj) const
{
	return ReflectedValue{ reinterpret_cast<u8*>(obj.GetPtr()) + m_Offset, m_Type };
}

ConstReflectedValue PropertyInfo::Access(ConstClassValuePtr const& obj) const
{
	return ConstReflectedValue{ reinterpret_cast<u8 const*>(obj.GetPtr()) + m_Offset, m_Type };
}

const PropertyInfo* ClassTypeInfo::FindProperty(HashString name) const
{
	const PropertyInfo* result = nullptr;
	ForEachPropertyWithBreak([&name, &result](const PropertyInfo& prop)
	{
		if (name == prop.GetName())
		{
			result = &prop;
			return true;
		}
		return false;
	});

	return result;
}

ClassTypeInfo const& ClassTypeInfo::GetRuntimeType(ConstReflectedValue val) const
{
	if (IsA<BaseObject>())
	{
		return val.GetAs<BaseObject>().GetTypeInfo();
	}
	else
	{
		return *this;
	}
}

const PropertyInfo& ClassTypeInfo::FindPropertyChecked(HashString name) const
{
	auto* result = FindProperty(name);
	ZE_ASSERT(result);
	return *result;
}
