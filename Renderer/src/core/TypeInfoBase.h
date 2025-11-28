#pragma once

#include "core/CoreTypes.h"
#include "core/Core.h"
#include "core/Concepts.h"
#include "core/NameBase.h"
//#include "core/Utils.h"
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
struct TypeInfoHelper
{
};
//template<typename T>
//inline TypeInfo const& GetTypeInfo();

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

	TypeInfo const& GetRuntimeType() const;

	Ptr GetPtr() const
	{
		return m_Obj;
	}

	TReflectedValue<IsConst> Downcast()
	{
		return TReflectedValue<IsConst> (this->m_Obj, GetRuntimeType());
	}

	operator const void* () const
	{
		return m_Obj;
	}
	template<typename = std::enable_if_t<!IsConst>>
	operator void*() const
	{
		return m_Obj;
	}

	template<typename T>
	ApplyConst<T, IsConst>& GetAs() const;
	template<typename T>
	ApplyConst<T, IsConst>& GetAsUnchecked() const;

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

	TypeInfo(HashString name, size_t size, size_t alignment, ETypeCategory cat = ETypeCategory::BASIC, ETypeFlags flags = ETypeFlags(COPYABLE | MOVABLE | DEFAULT_CONSTRUCTIBLE));

	HashString const& GetName() const { return m_Name; }
	String GetNameStr() const { return m_Name.ToString(); }

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

	virtual void Serialize(class Serializer& serializer, void* val) const = 0;

	virtual TypeInfo const& GetRuntimeType(ConstReflectedValue val) const
	{
		ZE_ASSERT(val.GetType() == *this);
		return *this;
	}

	int GetSize() const {
		return m_Size;
	}

	int GetAlignment() const
	{
		return m_Alignment;
	}

	bool IsClass() const 
	{
		return m_Category == ETypeCategory::CLASS;
	}

	virtual void* NewOperator() const = 0;

	//location must have GetSize() bytes available
	virtual ReflectedValue Construct(void* location) const = 0;

private:
	HashString m_Name;
	ETypeCategory m_Category;
	int m_Size;
	int m_Alignment;

	ETypeFlags m_TypeFlags;
};


//template<typename T>
//struct TypeInfoHelper
//{
//	
//};

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

template<bool IsConst>
class TClassValuePtr;

using ClassValuePtr = TClassValuePtr<false>;
using ConstClassValuePtr = TClassValuePtr<true>;



