#pragma once

#include "container/Vector.h"
#include "Hash.h"
#include "NameBase.h"
#include "ClassTypeInfo.h"

inline const char* GetNameData(Name const& name)
{
	return name.c_str();
}

template<>
struct std::hash<Name>
{
	size_t operator()(const Name& name) const
	{
		return CombineHash(name.GetHash(), name.GetIndex());
	}
};

DECLARE_CLASS_TYPEINFO_EXT(Name);
