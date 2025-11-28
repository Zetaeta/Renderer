#pragma once

#include "TypeInfoBase.h"
#include "common/Serializer.h"

TypeInfo const* FindTypeInfo(HashString name);

template<typename T>
class BasicTypeInfo : public TypeInfo
{
	friend TypeInfoHelper<T>;
	BasicTypeInfo(HashString name)
		: TypeInfo(name, sizeof(T), alignof(T), ETypeCategory::BASIC)
	{

	}
public:

	void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override
	{
		assert(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<T>() = from.GetAs<T>();
	}

	ReflectedValue Construct(void* location) const override
	{
		T* value = new (location) T;
		return ReflectedValue {value, this};
	}

	void Move(ReflectedValue const& from, ReflectedValue const& to) const override
	{
		ZE_ASSERT(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<T>() = std::move(from.GetAs<T>());
	}

	void Serialize(class Serializer& serializer, void* val) const override
	{
		serializer << *reinterpret_cast<T*>(val);
	}

	void* NewOperator() const override
	{
		return new T;
	}
};

class VoidTypeInfo final : public TypeInfo
{
	friend TypeInfoHelper<void>;
	VoidTypeInfo()
		: TypeInfo("void", 0, 1, ETypeCategory::BASIC) {}

public:
	void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override {}

	void Move(ReflectedValue const& from, ReflectedValue const& to) const override {}

	void Serialize(class Serializer& serializer, void* val) const override {}

	ReflectedValue Construct(void* location) const override { return NoValue; }

	void* NewOperator() const override { return nullptr; }
};

#define IF_CT_TYPEID(x)

#define DECLARE_TYPEINFO(typ, TI)\
	template<>\
	struct TypeInfoHelper<typ>           \
	{                               \
		IF_CT_TYPEID(constexpr static u64 ID = #typ ""_hash;)\
		constexpr static auto const	NAME = Static(#typ);\
		static TI const s_TypeInfo;\
	};

DECLARE_TYPEINFO(void, VoidTypeInfo)

#define DECLARE_BASIC_TYPEINFO(typ) DECLARE_TYPEINFO(typ, BasicTypeInfo<typ>)
	//template<>\
	//struct TypeInfoHelper<typ>           \
	//{                               \
	//	constexpr static u64 ID = #typ ""_hash;\
	//	constexpr static auto const		NAME = Static(#typ);\
	//	static BasicTypeInfo<typ> const s_TypeInfo;\
	//};



#define BASIC_TYPES \
	X(int)\
	X(u32)\
	X(u64)\
	X(s64)\
	X(float)\
	X(double)\
	X(u8)\
	X(s8)\
	X(u16)\
	X(s16)\
	X(bool)\
	X(std::nullptr_t)\
	X(std::string)

#define BASIC_TYPES_WITH_NAME \
	BASIC_TYPES\
	X(Name)

#define X(name) DECLARE_BASIC_TYPEINFO(name);
BASIC_TYPES
#undef X

inline TypeInfo const& NullTypeInfo()
{
	return GetTypeInfo<std::nullptr_t>(nullptr);
}

template<typename T>
HashString GetTypeName()
{
	return GetTypeInfo<T>().GetName();
}

#define DEFINE_BASIC_TYPEINFO(typ)\
	 BasicTypeInfo<typ> const TypeInfoHelper<typ>::s_TypeInfo(#typ);
	//ATSTART(typ##TypeInfoDecl, {\
	//	ZE_ASSERT (g_TypeDB.find(TypeInfoHelper<typ>::ID) == g_TypeDB.end(), "Type %s was already defined", #typ);\
	//	g_TypeDB[TypeInfoHelper<typ>::ID] = std::make_unique<BasicTypeInfo<typ>>(#typ);\
	//});



#include "TypeInfo.inl"


