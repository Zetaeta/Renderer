#pragma once

class ClassTypeInfo;

#define DECLARE_STI_EXT(Class, Parent)\
public:\
	using Super = Parent;\
	ClassTypeInfo const& GetTypeInfo() const; \
	static ClassTypeInfo const& StaticClass();

#define DECLARE_STI_EXT_NOBASE(Class) DECLARE_STI_EXT(Class, void)
