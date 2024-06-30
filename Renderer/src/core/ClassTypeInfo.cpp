#include "ClassTypeInfo.h"

ReflectedValue PropertyInfo::Access(ClassValuePtr const& obj) const
{
	return ReflectedValue{ reinterpret_cast<u8*>(obj.GetPtr()) + m_Offset, m_Type };
}

ConstReflectedValue PropertyInfo::Access(ConstClassValuePtr const& obj) const
{
	return ConstReflectedValue{ reinterpret_cast<u8 const*>(obj.GetPtr()) + m_Offset, m_Type };
}
