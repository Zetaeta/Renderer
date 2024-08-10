#include "core/TypeInfo.h"
#include "core/BaseObject.h"

std::unordered_map<Name, TypeInfo*>& GetTypeDB()
{
	static std::unordered_map<Name, TypeInfo*> db;
	return db;
}

TypeInfo const* FindTypeInfo(Name name)
{
	auto const& db = GetTypeDB();
	auto		it = db.find(name);
	if (it == db.end())
	{
		return nullptr;
	}
	return it->second;
}

bool ClassTypeInfo::InheritsFrom(ClassTypeInfo const& cls) const
{
	return (cls == *this) || ((m_Parent != nullptr) && m_Parent->InheritsFrom(cls));
}

template <>
ClassTypeInfo const& TClassValuePtr<true>::GetRuntimeType() const
{
	if (GetType().IsA<BaseObject>())
	{
		return GetAs<BaseObject>().GetTypeInfo();
	}
	else
	{
		return GetType();
	}
}

template <>
ClassTypeInfo const& TClassValuePtr<false>::GetRuntimeType() const
{
	if (GetType().IsA<BaseObject>())
	{
		return GetAs<BaseObject>().GetTypeInfo();
	}
	else
	{
		return GetType();
	}
}


#define X(name) DEFINE_BASIC_TYPEINFO(name);
BASIC_TYPES
#undef X

VoidTypeInfo const TypeInfoHelper<void>::s_TypeInfo;

 TypeInfo::TypeInfo(String name, size_t size, ETypeCategory cat /*= ETypeCategory::BASIC*/, ETypeFlags flags)
	: m_Name(name), m_Category(cat), m_Size(size), m_TypeFlags(flags)
{
	GetTypeDB()[name] = this;
 }

Vector<ClassTypeInfo const*> const& ClassTypeInfo::GetAllChildren() const
{
	if (!m_FoundChildren || m_AllChildren.size() < m_Children.size())
	{
		m_AllChildren.insert(m_AllChildren.end(), m_Children.begin(), m_Children.end());
		for (auto const& child : m_Children)
		{
			auto const& grandchildren = child->GetAllChildren();
			m_AllChildren.insert(m_AllChildren.end(), grandchildren.begin(), grandchildren.end());
		}
	}
	return m_AllChildren;
}

Vector<ClassTypeInfo const*> const& ClassTypeInfo::GetImmediateChildren() const
{
	if (!m_FoundChildren)
	{
		for (auto const& pr : GetTypeDB())
		{
			TypeInfo const* typ = pr.second;
			if (typ->IsClass())
			{
				auto const* cls = static_cast<ClassTypeInfo const*>(typ);
				if (cls->GetParent() == this)
				{
					m_Children.push_back(cls);
				}
			}
		}
		m_FoundChildren = true;
	}
	return m_Children;
}

ClassTypeInfo const& GetTypeInfo(BaseObject const& obj)
{
   return obj.GetTypeInfo();
}

ReflectedValue		NoValue{ nullptr, TypeInfoHelper<void>::s_TypeInfo };
ConstReflectedValue ConstNoValue{ nullptr, TypeInfoHelper<void>::s_TypeInfo };
