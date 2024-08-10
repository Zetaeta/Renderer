#pragma once

#include "core/Types.h"
#include "core/Utils.h"
#include "Hash.h"
#include <unordered_map>
#include <memory>
#include <functional>

using TypeID = u64;

enum class ETypeCategory : u8
{
	BASIC,
	CLASS,
	CONTAINER,
	POINTER
};

class TypeInfo;
class ClassTypeInfo;

template<typename T>
TypeInfo const& GetTypeInfo();

template<typename T>
TypeInfo const& GetTypeInfo(T const&)
{
	return GetTypeInfo<T>();
}

class BaseObject;
ClassTypeInfo const& GetTypeInfo(BaseObject const& obj);



template<bool IsConst>
class TReflectedValue
{
protected:
	using Ptr = ApplyConst<void, IsConst>*;
public:
	
	//TValuePtr(TValuePtr const& other) = default;
	//TValuePtr(TValuePtr&& other) = default;

	template<typename T>
	static TReflectedValue From(T& object)
	{
		return TReflectedValue(&object, GetTypeInfo(object));
	}

	template<typename T>
	static TReflectedValue From(T const& object)
		requires(IsConst)
	{
		return TReflectedValue(&object, GetTypeInfo<T>());
	}

	template<bool OthConst>
	TReflectedValue(TReflectedValue<OthConst> const& other) requires (IsConst || !OthConst)
		: m_Obj(other.GetPtr()), m_Type(&other.GetType())
	{
	}

	template<bool OthConst>
	TReflectedValue& operator=(TReflectedValue<OthConst> const& other) requires (IsConst || !OthConst)
	{
		m_Obj = other.GetPtr();
		m_Type = &other.GetType();
		return *this;
	}
	
	TReflectedValue(Ptr object, TypeInfo const& typ)
		: m_Obj(object), m_Type(&typ)
	{
	}

	TReflectedValue(Ptr object, TypeInfo const* typ)
		: m_Obj(object), m_Type(typ)
	{
	}

	TypeInfo const& GetType() const
	{
		return *m_Type;
	}

	virtual TypeInfo const& GetRuntimeType() const;

	Ptr GetPtr() const
	{
		return m_Obj;
	}

	TReflectedValue<IsConst> Downcast()
	{
		return TReflectedValue<IsConst> (this->m_Obj, GetRuntimeType());
	}


	template<typename T>
	ApplyConst<T, IsConst>& GetAs() const;

	TReflectedValue<false> ConstCast()
	{
		return TReflectedValue<false> {const_cast<void*>(m_Obj), m_Type};
	}
	
	bool IsNull()
	{
		return m_Obj == nullptr;
	}

	operator bool() { return m_Obj != nullptr; }

protected:
	Ptr m_Obj;
	TypeInfo const* m_Type;
	//bool m_RTTI = false;
};

template <bool IsConst>
TypeInfo const& TReflectedValue<IsConst>::GetRuntimeType() const
{
	return GetType().GetRuntimeType(*this);
}

using ReflectedValue = TReflectedValue<false>;
using ConstReflectedValue = TReflectedValue<true>;


extern ReflectedValue NoValue;
extern ConstReflectedValue ConstNoValue;

class TypeInfo
{
public:
	enum ETypeFlags
	{
		NONE = 0,
		COPYABLE = 0x1,
		MOVABLE = 0x2,
		DEFAULT_CONSTRUCTIBLE = 0x4,
	};

	RCOPY_MOVE_PROTECT(TypeInfo);

	TypeInfo(String name, size_t size, ETypeCategory cat = ETypeCategory::BASIC, ETypeFlags flags = ETypeFlags(COPYABLE | MOVABLE | DEFAULT_CONSTRUCTIBLE));

	Name const& GetName() const { return m_Name; }

	ETypeCategory GetTypeCategory() const
	{
		return m_Category;
	}

	bool operator==(TypeInfo const& other) const
	{
		return this == &other;
	}

	virtual void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const = 0;
	virtual void Move(ReflectedValue const& from, ReflectedValue const& to) const = 0;

	virtual bool Contains(TypeInfo const& cls) const {
		return cls == *this;
	}

	ETypeFlags GetFlags() const { return m_TypeFlags; }
	bool	   HasFlag(ETypeFlags flag) const { return m_TypeFlags & flag; }

	bool IsDefaultConstructible() const { return HasFlag(DEFAULT_CONSTRUCTIBLE); }
	bool IsCopyable() const { return HasFlag(COPYABLE); }
	bool IsMovable() const { return HasFlag(MOVABLE); }

	virtual TypeInfo const& GetRuntimeType(ConstReflectedValue val) const
	{
		RASSERT(val.GetType() == *this);
		return *this;
	}

	size_t GetSize() const {
		return m_Size;
	}

	bool IsClass() const 
	{
		return m_Category == ETypeCategory::CLASS;
	}


	//location must have GetSize() bytes available
	virtual ReflectedValue Construct(void* location) const = 0;

private:
	Name m_Name;
	ETypeCategory m_Category;
	size_t m_Size;

	ETypeFlags m_TypeFlags;
};


template<typename T>
struct TypeInfoHelper
{
	
};

//template<typename T>
//struct TypeInfoHelper
//{
//	
//};

template<typename T>
concept HasInternalTypeInfo = requires(T a)
{
	T::TypeInfoHelper::s_TypeInfo;
};

template<typename T>
concept HasExternalTypeInfo = requires(T a)
{
	TypeInfoHelper<T>::s_TypeInfo;
};

template<typename T>
inline TypeInfo const& GetTypeInfo()
	requires(HasExternalTypeInfo<T>)
{
	return TypeInfoHelper<std::remove_cvref_t<T>>::s_TypeInfo;// g_TypeDB[TypeInfoHelper<T>::ID].Get();
}

template<typename T>
	requires(HasInternalTypeInfo<T>)
inline TypeInfo const& GetTypeInfo()
{
	return std::remove_cvref_t<T>::TypeInfoHelper::s_TypeInfo;// g_TypeDB[TypeInfoHelper<T>::ID].Get();
}

template<typename T>
concept HasTypeInfo = HasInternalTypeInfo<T> || HasExternalTypeInfo<T>;

template<typename T>
concept HasClassTypeInfo = requires(T a)
{
	{ T::TypeInfoHelper::s_TypeInfo } -> DerivedStrip<ClassTypeInfo>;
};

template<typename T>
struct GetTypeInfoHelper_
{
};

template<typename T>
	requires (HasInternalTypeInfo<T>)
struct GetTypeInfoHelper_<T>
{
	using Type = T::TypeInfoHelper;
};

template<typename T>
	requires (HasExternalTypeInfo<T>)
struct GetTypeInfoHelper_<T>
{
	using Type = ::TypeInfoHelper<T>;
};

template<typename T>
using GetTypeInfoHelper = GetTypeInfoHelper_<T>::Type;


TypeInfo const* FindTypeInfo(Name name);

template<typename T>
class BasicTypeInfo : public TypeInfo
{
	friend TypeInfoHelper<T>;
	BasicTypeInfo(Name name)
		: TypeInfo(name, sizeof(T), ETypeCategory::BASIC) {

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
		RASSERT(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<T>() = std::move(from.GetAs<T>());
	}
};

class VoidTypeInfo final : public TypeInfo
{
	friend TypeInfoHelper<void>;
	VoidTypeInfo()
		: TypeInfo("void", 0, ETypeCategory::BASIC) {}

public:
	void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override {}

	void Move(ReflectedValue const& from, ReflectedValue const& to) const override {}

	ReflectedValue Construct(void* location) const override { return NoValue; }
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
	X(std::string)\

#define X(name) DECLARE_BASIC_TYPEINFO(name);
BASIC_TYPES
#undef X

//DECLARE_BASIC_TYPEINFO(int);
//DECLARE_BASIC_TYPEINFO(u32);
//DECLARE_BASIC_TYPEINFO(u64);
//DECLARE_BASIC_TYPEINFO(s64);
//DECLARE_BASIC_TYPEINFO(float);
//DECLARE_BASIC_TYPEINFO(double);
//DECLARE_BASIC_TYPEINFO(u8);
//DECLARE_BASIC_TYPEINFO(s8);
//DECLARE_BASIC_TYPEINFO(u16);
//DECLARE_BASIC_TYPEINFO(s16);
//DECLARE_BASIC_TYPEINFO(bool);

template<typename T>
String GetTypeName()
{
	return GetTypeInfo<T>().GetName();
}

#define DEFINE_BASIC_TYPEINFO(typ)\
	 BasicTypeInfo<typ> const TypeInfoHelper<typ>::s_TypeInfo(#typ);
	//ATSTART(typ##TypeInfoDecl, {\
	//	RASSERT(g_TypeDB.find(TypeInfoHelper<typ>::ID) == g_TypeDB.end(), "Type %s was already defined", #typ);\
	//	g_TypeDB[TypeInfoHelper<typ>::ID] = std::make_unique<BasicTypeInfo<typ>>(#typ);\
	//});



#include "TypeInfo.inl"


