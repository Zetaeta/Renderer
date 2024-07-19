#pragma once
#include "core/ClassTypeInfo.h"

class ClassTypeInfo;

class BaseObject
{
public:
	using Super = void;
	DECLARE_CLASS_TYPEINFO_(BaseObject);
	virtual ClassTypeInfo const& GetTypeInfo() const;

	static ClassTypeInfo const& GetStaticTypeInfo();

};

