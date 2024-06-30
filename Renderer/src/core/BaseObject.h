#pragma once
#include "core/ClassTypeInfo.h"

class ClassTypeInfo;

class BaseObject
{
public:
	using Super = void;
	virtual ClassTypeInfo const& GetTypeInfo() const;

	static ClassTypeInfo const& GetStaticTypeInfo();

};

DECLARE_CLASS_TYPEINFO(BaseObject);