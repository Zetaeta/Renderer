#pragma once
#include "core/TypeInfoBase.h"
#include "core/Maths.h"
#include "container/Span.h"

enum class ESerializerFlags : u8
{
	None = 0,
	Loading = 1 << 0,
	Storing = 1 << 1
};

FLAG_ENUM(ESerializerFlags)

using NoneTypeInfoType = std::nullptr_t;

class Serializer
{
public:
	Serializer(ESerializerFlags flags)
		: mFlags(flags) {}

#define X(type)\
	virtual Serializer& operator<<(type& val)\
	{\
		BinarySerializePrimitive(val);\
		return *this;\
	}

	PRIMITIVE_TYPES

#undef X

	Serializer& operator<<(NoneTypeInfoType)
	{
		ZE_ASSERT(false);
		return *this;
	}

	Serializer& operator<<(String& str)
	{
		SerializeString(str);
		return *this;
	}

	template<HasTypeInfo T>
	Serializer& operator<<(T& value)
	{
		GetTypeInfo<T>(value).Serialize(*this, &value);
		return *this;
	}


	template<typename T>
	void BinarySerializePrimitive(T& prim)
	{
		SerializeMemory(Span<uint8>(reinterpret_cast<uint8*>(&prim), sizeof(T)));
	}

	virtual void SerializeString(String& str);

	virtual void SerializeMemory(Span<uint8>){ ZE_ASSERT(false); };

	virtual void EnterArray(int& ArrayNum) = 0;

	virtual void LeaveArray() {}

	virtual void EnterStringMap() = 0;
	virtual void SerializeMapKey(std::string_view str) {}
	// Returned view is guaranteed to be valid until next call to ReadNextMapKey/LeaveStringMap
	virtual bool ReadNextMapKey(std::string_view& key) { return {}; }

	virtual void EnterPointer(const TypeInfo*& innerType) = 0;
	virtual void LeavePointer() {}
	virtual void LeaveStringMap() {}

	bool IsLoading() const
	{
		return !!(mFlags & ESerializerFlags::Loading);
	}

	bool IsStoring() const
	{
		return !!(mFlags & ESerializerFlags::Storing);
	}

	bool ShouldSerialize(const class PropertyInfo& prop);

protected: 
	virtual void SerializeGeneralClass(ClassValuePtr value);
	ESerializerFlags mFlags = ESerializerFlags::None;

	template<typename T>
	friend class ClassTypeInfoImpl;
};
