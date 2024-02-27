#pragma once

#include "Types.h"
#include "Utils.h"
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
class TValuePtr
{
protected:
	using Ptr = ApplyConst<void, IsConst>*;
public:
	
	//TValuePtr(TValuePtr const& other) = default;
	//TValuePtr(TValuePtr&& other) = default;

	template<typename T>
	static TValuePtr From(T& object)
	{
		return TValuePtr(&object, GetTypeInfo(object));
	}

	template<typename T>
	static TValuePtr From(T const& object)
		requires(IsConst)
	{
		return TValuePtr(&object, GetTypeInfo<T>());
	}

	template<bool OthConst>
	TValuePtr(TValuePtr<OthConst> const& other) requires (IsConst || !OthConst)
		: m_Obj(other.GetPtr()), m_Type(&other.GetType())
	{
	}

	template<bool OthConst>
	TValuePtr& operator=(TValuePtr<OthConst> const& other) requires (IsConst || !OthConst)
	{
		m_Obj = other.GetPtr();
		m_Type = &other.GetType();
		return *this;
	}
	
	TValuePtr(Ptr object, TypeInfo const& typ)
		: m_Obj(object), m_Type(&typ)
	{
	}

	TValuePtr(Ptr object, TypeInfo const* typ)
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

	TValuePtr<IsConst> Downcast()
	{
		return TValuePtr<IsConst> (this->m_Obj, GetRuntimeType());
	}


	template<typename T>
	ApplyConst<T, IsConst>& GetAs() const;

	TValuePtr<false> ConstCast()
	{
		return TValuePtr<false> {const_cast<void*>(m_Obj), m_Type};
	}

protected:
	Ptr m_Obj;
	TypeInfo const* m_Type;
	//bool m_RTTI = false;
};

template <bool IsConst>
TypeInfo const& TValuePtr<IsConst>::GetRuntimeType() const
{
	return GetType().GetRuntimeType(*this);
}

using ValuePtr = TValuePtr<false>;
using ConstValuePtr = TValuePtr<true>;



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

	virtual void Copy(ConstValuePtr const& from, ValuePtr const& to) const = 0;
	virtual void Move(ValuePtr const& from, ValuePtr const& to) const = 0;

	virtual bool Contains(TypeInfo const& cls) const {
		return cls == *this;
	}

	ETypeFlags GetFlags() const { return m_TypeFlags; }
	bool	   HasFlag(ETypeFlags flag) const { return m_TypeFlags & flag; }

	bool IsDefaultConstructible() const { return HasFlag(DEFAULT_CONSTRUCTIBLE); }
	bool IsCopyable() const { return HasFlag(COPYABLE); }
	bool IsMovable() const { return HasFlag(MOVABLE); }

	virtual TypeInfo const& GetRuntimeType(ConstValuePtr val) const
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
	virtual ValuePtr Construct(void* location) const = 0;

private:
	Name m_Name;
	ETypeCategory m_Category;
	size_t m_Size;

	ETypeFlags m_TypeFlags;
};


class AttrAccessor;

class ClassTypeInfo;

template<bool IsConst>
class TClassValuePtr : public TValuePtr<IsConst>
{
	using Ptr = ApplyConst<void, IsConst>*;
public:
	
	template<typename T>
	TClassValuePtr(ApplyConst<T, IsConst>& object)
		requires(std::is_class_v<T>)
		: TValuePtr<IsConst>(object)
	{
	}

	template<bool OthConst>
	TClassValuePtr(TClassValuePtr<OthConst> const& other) requires (IsConst || !OthConst)
		: TValuePtr<IsConst>(other)
	{
	}

	template<bool OthConst>
	TClassValuePtr& operator=(TClassValuePtr<OthConst> const& other) requires (IsConst || !OthConst)
	{
		this->m_Obj = other.m_Obj;
		this->m_Type = other.m_Type;
		return *this;
	}
	
	TClassValuePtr(Ptr object, ClassTypeInfo const& typ)
		: TValuePtr<IsConst>(object, typ)
	{
	}

	template<typename T>
	static TClassValuePtr From(T& object)
	{
		return TClassValuePtr(&object, static_cast<ClassTypeInfo const&>(GetTypeInfo(object)));
	}

	TClassValuePtr<IsConst> Downcast()
	{
		return TClassValuePtr<IsConst> (this->m_Obj, GetRuntimeType());
	}

	ClassTypeInfo const& GetRuntimeType() const;

	//TClassValuePtr(Ptr object, ClassTypeInfo const* typ)
	//	: TValuePtr(object, typ)
	//{
	//}

	TClassValuePtr(TValuePtr<IsConst> val)
		:TClassValuePtr ( val.GetPtr(), static_cast<ClassTypeInfo const&>(val.GetType()) )
	{
		RASSERT(val.GetType().GetTypeCategory() == ETypeCategory::CLASS);
	}

	ClassTypeInfo const& GetType() const
	{
		//return *static_cast<ClassTypeInfo const*>(m_Type);
		return static_cast<ClassTypeInfo const&>(TValuePtr<IsConst>::GetType());
	}
};

using ClassValuePtr = TClassValuePtr<false>;
using ConstClassValuePtr = TClassValuePtr<true>;

class PropertyInfo
{
protected:
	Name m_Name;
	TypeInfo const* m_Type;
	bool m_Const;

	PropertyInfo(Name name, TypeInfo const& type, bool isConst = false)
		: m_Name(name), m_Type(&type), m_Const(isConst) {}

public:
	Name GetName() const { return m_Name; }
	TypeInfo const&		  GetType() const { return *m_Type; }
	bool				  IsConst() const { return m_Const; }
	virtual ValuePtr Access(ClassValuePtr const& obj) const = 0;
	virtual ConstValuePtr Access(ConstClassValuePtr const& obj) const = 0;
};

class ClassTypeInfo : public TypeInfo
{
public:
	ClassTypeInfo(Name name, size_t size, ClassTypeInfo const* parent, ETypeFlags typeFlags, Vector<OwningPtr<PropertyInfo>>&& attrs)
		: TypeInfo(name, size, ETypeCategory::CLASS, typeFlags), m_Parent(parent), m_Properties(std::move(attrs)) {}

	virtual bool Contains(TypeInfo const& cls) const override {
		return cls.GetTypeCategory() == ETypeCategory::CLASS && InheritsFrom(static_cast<ClassTypeInfo const&>(cls));
	}

	bool InheritsFrom(ClassTypeInfo const& cls) const;

	template<typename T>
	bool IsA() const
	{
		if constexpr (!std::is_class_v<T>)
		{
			return false;
		}
		else
		{
			return InheritsFrom(GetClassTypeInfo<T>());
		}
	}

	ClassTypeInfo const& GetRuntimeType(ConstValuePtr val) const override
	{
		RASSERT(val.GetType() == *this);
		return ConstClassValuePtr(val).GetRuntimeType();
	}


	ClassTypeInfo const* GetParent() const { return m_Parent; }

	template<typename TFunc>
	void ForEachProperty(TFunc const& func) const
	{
		if (m_Parent != nullptr)
		{
			m_Parent->ForEachProperty(func);
		}
		for (auto const& prop : m_Properties)
		{
			func(prop.get());
		}
	}

	Vector<ClassTypeInfo const*> const& GetAllChildren() const;
	Vector<ClassTypeInfo const*> const& GetImmediateChildren() const;

protected:
	ClassTypeInfo const* m_Parent;
	Vector<OwningPtr<PropertyInfo>> m_Properties;
	mutable Vector<ClassTypeInfo const*> m_Children;
	mutable Vector<ClassTypeInfo const*> m_AllChildren;
	mutable bool m_FoundChildren = false;

public:
};

template<typename T>
inline TypeInfo::ETypeFlags ComputeFlags()
{
	u32 flags = TypeInfo::NONE;
	if constexpr (std::copyable<T>)
	{
		flags |= TypeInfo::COPYABLE;
	}
	if constexpr (std::movable<T>)
	{
		flags |= TypeInfo::MOVABLE;
	}
	if constexpr (std::is_default_constructible_v<T>)
	{
		flags |= TypeInfo::DEFAULT_CONSTRUCTIBLE;
	}
	return static_cast<TypeInfo::ETypeFlags>(flags);
}

template<typename TClass>
class ClassTypeInfoImpl : public ClassTypeInfo
{
public:
	ClassTypeInfoImpl(Name name, ClassTypeInfo const* parent, Vector<OwningPtr<PropertyInfo>>&& attrs)
		: ClassTypeInfo(name, sizeof(TClass), parent, ComputeFlags<TClass>(), std::move(attrs)) {}

	ValuePtr Construct(void* location) const override
	{
		if constexpr (std::constructible_from<TClass>)
		{
			TClass* value = new (location) TClass;
			return ValuePtr {value, this};
		}
		else
		{
			RASSERT(false);
			return ValuePtr {nullptr, this};
		}
	}

	void Move(ValuePtr const& from, ValuePtr const& to) const override
	{
		RASSERT(from.GetType().GetTypeCategory() == ETypeCategory::CLASS && static_cast<ClassTypeInfo const&>(from.GetType()).InheritsFrom(*this) && to.GetType() == *this, "Incompatible types");
		if constexpr (std::movable<TClass>)
		{
			to.GetAs<TClass>() = std::move(from.GetAs<TClass>());
		}
		else
		{
			RASSERT(false, "Not movable");
		}
	}

	void Copy(ConstValuePtr const& from, ValuePtr const& to) const override
	{
		RASSERT(from.GetType().GetTypeCategory() == ETypeCategory::CLASS && static_cast<ClassTypeInfo const&>(from.GetType()).InheritsFrom(*this) && to.GetType() == *this, "Incompatible types");
		if constexpr (std::copyable<TClass>)
		{
			to.GetAs<TClass>() = from.GetAs<TClass>();
		}
		else
		{
			RASSERT(false, "Not copyable");
		}
	}
};



template<typename TParent, typename TChild>
using MemberPtr = TChild TParent::*;

template<typename TParent, typename TChild>
class AttrAccessorImpl : public PropertyInfo
{
public:
	AttrAccessorImpl(Name name, MemberPtr<TParent, TChild> member) :PropertyInfo(name, GetTypeInfo<TChild>()), m_Member(member) {}
	virtual ValuePtr Access(ClassValuePtr const& obj) const override
	{
		RASSERT(obj.GetType().IsA<TParent>());
		TChild* member = &(obj.GetAs<TParent>().*m_Member);
		return ValuePtr {member, *m_Type};
	}

	virtual ConstValuePtr Access(ConstClassValuePtr const& obj) const override
	{
		RASSERT(obj.GetType().IsA<TParent>());
		TChild const* member = &(obj.GetAs<TParent>().*m_Member);
		return ConstValuePtr {member, *m_Type};
	}

private:
	MemberPtr<TParent, TChild> m_Member;
};


template<typename T>
struct TypeInfoHelper
{
	
};

template<typename T>
TypeInfo const& GetTypeInfo()
{
	return TypeInfoHelper<T>::s_TypeInfo;// g_TypeDB[TypeInfoHelper<T>::ID].Get();
}

TypeInfo const* FindTypeInfo(Name name);

//template<>
//TypeInfo const& GetTypeInfo<void>()
//{
//	static TypeInfo s_Void {"void", ETypeCategory::BASIC};
//	return s_Void;
//}

template<typename T>
ClassTypeInfo const* MaybeGetClassTypeInfo()
{
	//if constexpr (std::is_same_v<T,void>)
	//{
	//	return nullptr;
	//}
	if constexpr (std::is_class_v<T>)
	{
		return static_cast<ClassTypeInfo const*>(&GetTypeInfo<T>());
	}
	return nullptr;
}

template<typename T>
ClassTypeInfo const& GetClassTypeInfo()
{
	//if constexpr (std::is_same_v<T,void>)
	//{
	//	return nullptr;
	//}
	static_assert(std::is_class_v<T>, "T is not a class");
	return *MaybeGetClassTypeInfo<T>();
}


//template<class T>
//std::enable_if_t<std::is_class_v<T>, ClassTypeInfo const&> GetClassTypeInfo()
//{
//	
//}

template<typename TFunc>
struct ConstructorHelper
{
	ConstructorHelper(TFunc&& f)
	{
		f();
	}
};

#define ATSTART(name, block) \
static auto s_##name = ConstructorHelper([] { block; });

#define DECLARE_CLASS_TYPEINFO(typ)\
	template<>\
	DECLARE_CLASS_TYPEINFO_TEMPLATE(typ)
#define DECLARE_CLASS_TYPEINFO_TEMPLATE(typ)\
	struct TypeInfoHelper<typ>           \
	{                               \
		constexpr static u64 ID = #typ ""_hash;\
		constexpr static auto const NAME = Static(#typ);\
		using Type = typ;\
		using MyClassInfo = ClassTypeInfoImpl<Type>;\
		static MyClassInfo		MakeTypeInfo();\
		static MyClassInfo const	s_TypeInfo;\
	};

#define DECLARE_STI(Class, Parent) friend struct TypeInfoHelper<Class>;\
public:\
	using Super = Parent;\
	ClassTypeInfo const& GetTypeInfo() const; \
	static ClassTypeInfo const& GetStaticTypeInfo(); 

#define DECLARE_RTTI(Class, Parent) friend struct TypeInfoHelper<Class>;\
public:\
	using Super = Parent;\
	virtual ClassTypeInfo const& GetTypeInfo() const override;\
	static ClassTypeInfo const& GetStaticTypeInfo();

#define DECLARE_STI_NOBASE(Class) DECLARE_STI(Class, void)

#define DEFINE_CLASS_TYPEINFO_TEMPLATE(temp, Class)\
	temp\
	ClassTypeInfo const& Class::GetTypeInfo() const\
	{                                  \
		return ::GetClassTypeInfo<Class>();\
	}\
	temp\
	ClassTypeInfo const& Class::GetStaticTypeInfo() \
	{                                        \
		return ::GetClassTypeInfo<Class>();\
	}\
	temp\
	ClassTypeInfoImpl<Class> const TypeInfoHelper<Class>::s_TypeInfo = MakeTypeInfo();\
	temp\
	ClassTypeInfoImpl<Class> TypeInfoHelper<Class>::MakeTypeInfo() {\
		Name name = #Class;\
		Vector<OwningPtr<PropertyInfo>> attrs;\
		auto const* parent = MaybeGetClassTypeInfo<Class::Super>();\

#define DEFINE_CLASS_TYPEINFO(Class)\
	ClassTypeInfo const& Class::GetTypeInfo() const\
	{                                  \
		return ::GetClassTypeInfo<Class>();\
	}\
	ClassTypeInfo const& Class::GetStaticTypeInfo() \
	{                                        \
		return ::GetClassTypeInfo<Class>();\
	}\
	ClassTypeInfoImpl<Class> const TypeInfoHelper<Class>::s_TypeInfo = MakeTypeInfo();\
	ClassTypeInfoImpl<Class> TypeInfoHelper<Class>::MakeTypeInfo() {\
		Name name = #Class;\
		Vector<OwningPtr<PropertyInfo>> attrs;\
		auto const* parent = MaybeGetClassTypeInfo<Class::Super>();\
	//= ConstructorHelper < std::function<void()>([] {\
	//g_TypeDB[TypeInfoHelper<Class>::ID] = std::make_unique<ClassTypeInfo>(#Class, GetTypeInfo<Class::Super>(), Vector<OwningPtr<AttributeInfo>> { 

#define BEGIN_REFL_PROPS()
#define REFL_PROP(prop) attrs.emplace_back(std::make_unique < AttrAccessorImpl<Type, decltype(Type::prop)>>(#prop, &Type::prop));
#define END_REFL_PROPS()
#define END_CLASS_TYPEINFO()                           \
	return ClassTypeInfoImpl<Type>{ name, parent, std::move(attrs) }; \
	}

template<typename T>
class BasicTypeInfo : public TypeInfo
{
public:
	BasicTypeInfo(Name name)
		: TypeInfo(name, sizeof(T), ETypeCategory::BASIC) {

	}

	void Copy(ConstValuePtr const& from, ValuePtr const& to) const override
	{
		assert(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<T>() = from.GetAs<T>();
	}

	ValuePtr Construct(void* location) const override
	{
		T* value = new (location) T;
		return ValuePtr {value, this};
	}

	void Move(ValuePtr const& from, ValuePtr const& to) const override
	{
		RASSERT(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<T>() = std::move(from.GetAs<T>());
	}
};

#define DECLARE_BASIC_TYPEINFO(typ)\
	template<>\
	struct TypeInfoHelper<typ>           \
	{                               \
		constexpr static u64 ID = #typ ""_hash;\
		constexpr static auto const		NAME = Static(#typ);\
		static BasicTypeInfo<typ> const s_TypeInfo;\
	};

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

#include "PointerTypeInfo.h"
#include "ContainerTypeInfo.h"
