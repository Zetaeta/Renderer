#pragma once

#include "container/Vector.h"
#include "Hash.h"
#include "NameBase.h"
#include "ClassTypeInfo.h"

inline const char* GetNameData(HashString const& name)
{
	return name.c_str();
}

template<>
struct std::hash<HashString>
{
	size_t operator()(const HashString& name) const
	{
		return CombineHash(name.GetHash(), name.GetIndex());
	}
};

DECLARE_CLASS_TYPEINFO_EXT(HashString);
