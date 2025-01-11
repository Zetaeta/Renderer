#pragma once
#include "core/ClassTypeInfo.h"

class ClassTypeInfo;

class BaseObject
{
public:
	using Super = void;
	DECLARE_CLASS_TYPEINFO_(BaseObject);
	virtual ClassTypeInfo const& GetTypeInfo() const;
	template<typename T>
	bool IsA() const
		requires HasClassTypeInfo<T>
	{
		return GetTypeInfo().InheritsFrom(T::StaticClass());
	}

	static ClassTypeInfo const& StaticClass();

};

//template<typename To, typename From>
//To* Cast(From* from)
//{
//	if (from->IsA<To>())
//	{
//		return static_cast<To>(from);
//	}
//	else
//	{
//		return nullptr;
//	}
//}
//
//template<typename To, typename From>
//To const* Cast(From const* from)
//{
//	if (from->IsA<To>())
//	{
//		return static_cast<To>(from);
//	}
//	else
//	{
//		return nullptr;
//	}
//}

