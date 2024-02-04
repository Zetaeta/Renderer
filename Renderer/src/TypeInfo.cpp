#include "TypeInfo.h"


#include "TypeInfo.h"

std::unordered_map<TypeID, std::unique_ptr<TypeInfo>> g_TypeDB;

bool ClassTypeInfo::InheritsFrom(ClassTypeInfo const& cls) const
{
	return (m_Parent != nullptr) && m_Parent->InheritsFrom(cls);
}
