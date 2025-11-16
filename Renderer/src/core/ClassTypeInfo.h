#pragma once
#include "core/TypeInfo.h"
#include "core/TypeTraits.h"
#include <variant>
#include <array>
#include <optional>
#include <span>
#include "container/Vector.h"


#define FOR_EACH_METADATA(X) \
	X(MaxValue, double)\
	X(MinValue, double)\
	X(DragSpeed, double)\
	X(AutoExpand, bool)\

enum class EMemberMetadata : u32
{
	MaxValue,
	MinValue,
	DragSpeed,
	AutoExpand,
	Count,
	None = Count
};

class MemberMetadata
{
//	ZE_COPY_PROTECT(MemberMetadata);
public:
//	RMOVE_DEFAULT(MemberMetadata);

	using DatumValue = std::variant<int,double,String,bool>;
	class Datum
	{
	public:
		constexpr Datum(EMemberMetadata type = EMemberMetadata::None, DatumValue value = {})
			: m_Type(type), m_Value(value) {}
		EMemberMetadata m_Type;
		DatumValue m_Value;

		bool IsSet()
		{
			return m_Type != EMemberMetadata::None;
		}

	};

	template<typename T>
	class DatumName
	{
	public:
		constexpr DatumName(EMemberMetadata type)
			: m_Type(type) {}

		EMemberMetadata m_Type;

		constexpr Datum operator=(T const& value) const
		{
			return Datum {m_Type, value};
		}
	};

	template<>
	class DatumName<bool>
	{
	public:
		constexpr DatumName(EMemberMetadata type)
			: m_Type(type) {}
		EMemberMetadata m_Type;

		constexpr Datum operator=(bool value) const
		{
			return Datum {m_Type, value};
		}

		constexpr operator Datum() const
		{
			return Datum(m_Type, true);
		}
	};

	#define DECLARE_GETTER(Name, Type) \
	std::optional<Type> Get##Name() const\
	{\
		return IsSet(EMemberMetadata::Name) ? std::optional<Type>(std::get<Type>(m_Data[Denum(EMemberMetadata::Name)].m_Value))\
											: std::nullopt;\
	}

	FOR_EACH_METADATA(DECLARE_GETTER)

	constexpr MemberMetadata() = default;

	template<typename... Args>
	constexpr MemberMetadata(Datum&& arg1, Args... args)
	{
		Set(std::move(arg1));
	}

	constexpr bool IsSet(EMemberMetadata type) const
	{
		return m_Data[Denum(type)].m_Type == type;
		//return std::visit([type](auto const& datum) -> bool { return datum.m_Type == type; }, m_Data[Denum(type)]);
	}

private:
	constexpr void Set(Datum&& datum)
	{
		ZE_ASSERT(!IsSet(datum.m_Type));

		m_Data[Denum(datum.m_Type)] = std::move(datum);
	}
	std::array<Datum, Denum(EMemberMetadata::Count)> m_Data;
};

#define DECLARE_MD_NAME(Name, type, ...) \
constexpr MemberMetadata::DatumName<type> Name {EMemberMetadata::Name};

FOR_EACH_METADATA(DECLARE_MD_NAME)
#undef DECLARE_MD_NAME

#define META(...) MemberMetadata(__VA_ARGS__)

//MemberMetadata::DatumName<double> MaxValue {EMemberMetadata::MaxValue};
//MemberMetadata::DatumName<double> MaxValue {EMemberMetadata::MaxValue};

class AttrAccessor;

class ClassTypeInfo;

template<bool IsConst>
class TClassValuePtr : public TReflectedValue<IsConst>
{
	using Ptr = ApplyConst<void, IsConst>*;
public:
	
	template<typename T>
	TClassValuePtr(ApplyConst<T, IsConst>& object)
		requires(std::is_class_v<T>)
		: TReflectedValue<IsConst>(object)
	{
	}

	template<bool OthConst>
	TClassValuePtr(TClassValuePtr<OthConst> const& other) requires (IsConst || !OthConst)
		: TReflectedValue<IsConst>(other)
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
		: TReflectedValue<IsConst>(object, typ)
	{
	}

	template<typename T>
	static TClassValuePtr From(T& object)
		requires HasClassTypeInfo<std::remove_cvref_t<T>>
	{
		return TClassValuePtr(&object, static_cast<ClassTypeInfo const&>(GetTypeInfo(object)));
	}

	template<typename T>
	static TClassValuePtr From(T* object)
		requires HasClassTypeInfo<std::remove_cvref_t<T>>
	{
		return TClassValuePtr(object, static_cast<ClassTypeInfo const&>(GetTypeInfo(*object)));
	}

	TClassValuePtr<IsConst> Downcast()
	{
		return TClassValuePtr<IsConst> (this->m_Obj, GetRuntimeType());
	}

	ClassTypeInfo const& GetRuntimeType() const
	{
		return GetType().GetRuntimeType(*this);
	}

	//TClassValuePtr(Ptr object, ClassTypeInfo const* typ)
	//	: TValuePtr(object, typ)
	//{
	//}

	TClassValuePtr(TReflectedValue<IsConst> val)
		:TClassValuePtr ( val.GetPtr(), static_cast<ClassTypeInfo const&>(val.GetType()) )
	{
		ZE_ASSERT(val.GetType().GetTypeCategory() == ETypeCategory::CLASS);
	}

	ClassTypeInfo const& GetType() const
	{
		//return *static_cast<ClassTypeInfo const*>(m_Type);
		return static_cast<ClassTypeInfo const&>(TReflectedValue<IsConst>::GetType());
	}
};

using ClassValuePtr = TClassValuePtr<false>;
using ConstClassValuePtr = TClassValuePtr<true>;

class MemberFunction
{
public:
	virtual void Invoke(ReflectedValue object, ReflectedValue outReturn, std::span<ReflectedValue> const& arguments) = 0;
};

template<typename TClass, typename TRetVal, typename... TArgs>
using MemberFnPtr = TRetVal (TClass::*)(TArgs...);

template<typename TClass, typename TRetVal, typename... Args, size_t... Is>
TRetVal InvokeHelper(TClass& obj, MemberFnPtr<TClass, TRetVal, Args...> func, std::index_sequence<Is...>, std::span<ReflectedValue> const& args)
{
	return (obj.*func)(args[Is].GetAs<std::remove_cvref_t<Args>>()...);
}


template<typename TClass, typename TRetVal, typename... Args>
class MemberFunctionImpl : public MemberFunction
{
public:
	MemberFunctionImpl(MemberFnPtr<TClass, TRetVal, Args...> fn)
		: m_Function(fn) {}
	virtual void Invoke(ReflectedValue object, ReflectedValue outReturn, std::span<ReflectedValue> const& arguments) override
	{
		auto call = [&]
		{
			return InvokeHelper<TClass,TRetVal, Args...>(object.GetAs<TClass>(), m_Function, std::index_sequence_for<Args...>{}, arguments);
		};
		ZE_ASSERT(arguments.size() == sizeof...(Args), "Wrong number of arguments");
		if constexpr (std::is_same_v<void, TRetVal>)
		{
			call();
		}
		else
		{
			if (outReturn.IsNull())
			{
				call();
			}
			else
			{
				outReturn.GetAs<TRetVal>() = call();
			}
		}
	}
private:
	MemberFnPtr<TClass, TRetVal, Args...> m_Function;
};


class PropertyInfo
{
protected:
	Name m_Name;
	TypeInfo const* m_Type;
	bool m_Const;
	ptrdiff_t m_Offset;
	MemberMetadata m_Meta;
	MemberFunction* m_Setter = nullptr;

public:
	PropertyInfo(Name name, TypeInfo const& type, ptrdiff_t offset, bool isConst = false, MemberMetadata&& meta = {})
		: m_Name(name), m_Type(&type), m_Const(isConst), m_Meta(std::move(meta)), m_Offset(offset) {}

	PropertyInfo(Name name, TypeInfo const& type, ptrdiff_t offset, MemberFunction* setter, bool isConst = false, MemberMetadata&& meta = {})
		: m_Name(name), m_Type(&type), m_Const(isConst), m_Meta(std::move(meta)), m_Offset(offset), m_Setter(setter) {}

	Name GetName() const { return m_Name; }
	String GetNameStr() const
	{
		return m_Name.ToString();
	}

	TypeInfo const&		  GetType() const { return *m_Type; }
	bool				  IsConst() const { return m_Const; }
	ReflectedValue		  Access(ClassValuePtr const& obj) const;
	ConstReflectedValue Access(ConstClassValuePtr const& obj) const;
	MemberMetadata const& GetMetadata() const { return m_Meta; }

	bool HasSetter() const { return m_Setter != nullptr; }
	MemberFunction* GetSetter() const { return m_Setter; }

	ptrdiff_t GetOffset() const { return m_Offset; }
};

enum class EClassFlags : u8
{
	None = 0,
	Transient = 1,
};
FLAG_ENUM(EClassFlags)

class ClassTypeInfo : public TypeInfo
{
public:
	ClassTypeInfo(Name name, size_t size, size_t alignment, ClassTypeInfo const* parent, ETypeFlags typeFlags, EClassFlags classFlags, Vector<PropertyInfo>&& attrs)
		: TypeInfo(name, size, alignment, ETypeCategory::CLASS, typeFlags), m_Parent(parent), m_Properties(std::move(attrs)), mClassFlags(classFlags) {}

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

	ClassTypeInfo const& GetRuntimeType(ConstReflectedValue val) const override;

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
			func(prop);
		}
	}

	template<typename TFunc>
	bool ForEachPropertyWithBreak(TFunc const& func) const
	{
		if (m_Parent != nullptr)
		{
			if (m_Parent->ForEachPropertyWithBreak(func))
			{
				return true;
			}
		}
		for (auto const& prop : m_Properties)
		{
			if (func(prop))
			{
				return true;
			}
		}
		return false;
	}

	const PropertyInfo* FindProperty(Name name) const;
	const PropertyInfo& FindPropertyChecked(Name name) const;


	Vector<ClassTypeInfo const*> const& GetAllChildren() const;
	Vector<ClassTypeInfo const*> const& GetImmediateChildren() const;

	EClassFlags GetClassFlags() const { return mClassFlags; }
	bool HasAnyClassFlags(EClassFlags flags) const { return !!(mClassFlags & flags); }

	using Properties = Vector<PropertyInfo>;

protected:
	ClassTypeInfo const* m_Parent;
	Vector<PropertyInfo> m_Properties;
	mutable Vector<ClassTypeInfo const*> m_Children;
	mutable Vector<ClassTypeInfo const*> m_AllChildren;
	mutable bool m_FoundChildren = false;
	EClassFlags mClassFlags = EClassFlags::None;
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
	ClassTypeInfoImpl(Name name, ClassTypeInfo const* parent, EClassFlags classFlags, Properties&& attrs)
		: ClassTypeInfo(name, sizeof(TClass), alignof(TClass), parent, ComputeFlags<TClass>(), classFlags, std::move(attrs)) {}

	ReflectedValue Construct(void* location) const override
	{
		if constexpr (std::constructible_from<TClass>)
		{
			TClass* value = new (location) TClass;
			return ReflectedValue {value, this};
		}
		else
		{
			ZE_ASSERT(false);
			return ReflectedValue {nullptr, this};
		}
	}

	void Move(ReflectedValue const& from, ReflectedValue const& to) const override
	{
		ZE_ASSERT(from.GetType().GetTypeCategory() == ETypeCategory::CLASS && static_cast<ClassTypeInfo const&>(from.GetType()).InheritsFrom(*this) && to.GetType() == *this, "Incompatible types");
		if constexpr (std::movable<TClass>)
		{
			to.GetAs<TClass>() = std::move(from.GetAs<TClass>());
		}
		else
		{
			ZE_ASSERT(false, "Not movable");
		}
	}

	void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override
	{
		ZE_ASSERT(from.GetType().GetTypeCategory() == ETypeCategory::CLASS && static_cast<ClassTypeInfo const&>(from.GetType()).InheritsFrom(*this) && to.GetType() == *this, "Incompatible types");
		if constexpr (std::copyable<TClass>)
		{
			to.GetAs<TClass>() = from.GetAs<TClass>();
		}
		else
		{
			ZE_ASSERT(false, "Not copyable");
		}
	}

	void Serialize(class Serializer& serializer, void* val) const override
	{
		if constexpr (TClassTypeTraits<TClass>::HasSerialize)
		{
			reinterpret_cast<TClass*>(val)->Serialize(serializer);
		}
		else
		{
			serializer.SerializeGeneralClass(ClassValuePtr{val, *this});
		}
	}

	void* NewOperator() const override
	{
		if constexpr (std::constructible_from<TClass>)
		{
			return new TClass;
		}
		else
		{
			return nullptr;
		}
	}

};



template<typename TParent, typename TChild>
using MemberPtr = TChild TParent::*;


template<typename T>
ClassTypeInfo const* MaybeGetClassTypeInfo()
{
	if constexpr (std::is_class_v<T>)
	{
		return static_cast<ClassTypeInfo const*>(&GetTypeInfo<std::remove_cvref_t<T>>());
	}
	return nullptr;
}

template<typename T>
ClassTypeInfo const& GetClassTypeInfo()
{
	static_assert(std::is_class_v<T>, "T is not a class");
	return *MaybeGetClassTypeInfo<T>();
}

template<typename To, typename From>
To* Cast(From* from)
	requires std::is_base_of_v<std::remove_cvref_t<From>, std::remove_cvref_t<To>>
		&& std::is_base_of_v<BaseObject, std::remove_cvref_t<From>>
{
	if (from && from->IsA<To>())
	{
		return static_cast<To*>(from);
	}

	return nullptr;
}

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

#define DECLARE_CLASS_TYPEINFO_TEMPLATE(typ)
#define DECLARE_CLASS_TYPEINFO(typ)


//	template<>\
//	DECLARE_CLASS_TYPEINFO_TEMPLATE_(typ)
//#define DECLARE_CLASS_TYPEINFO_TEMPLATE_(typ)
#define DECLARE_CLASS_TYPEINFO_BODY(typ)\
	{                               \
		IF_CT_TYPEID(constexpr static u64 ID = #typ ""_hash;)\
		constexpr static auto const NAME = Static(#typ);\
		using Type = typ;\
		using MyClassInfo = ClassTypeInfoImpl<Type>;\
		static MyClassInfo		MakeTypeInfo();\
		static MyClassInfo const	s_TypeInfo;\
	};

#define DECLARE_CLASS_TYPEINFO_(typ) struct TypeInfoHelper\
	DECLARE_CLASS_TYPEINFO_BODY(typ)
#define DECLARE_CLASS_TYPEINFO_EXT(typ)\
	template<> struct TypeInfoHelper<typ>\
		DECLARE_CLASS_TYPEINFO_BODY(typ)



#define DECLARE_STI_EXTERNAL(Class, Parent)\
public:\
	using Super = Parent;\
	ClassTypeInfo const& GetTypeInfo() const; \
	static ClassTypeInfo const& StaticClass();

#define DECLARE_STI(Class, Parent)\
public:\
	using Super = Parent;\
	ClassTypeInfo const& GetTypeInfo() const; \
	static ClassTypeInfo const& StaticClass();\
	DECLARE_CLASS_TYPEINFO_(Class)

#define DECLARE_RTTI(Class, Parent)\
public:\
	using Super = Parent;\
	virtual ClassTypeInfo const& GetTypeInfo() const override;\
	static ClassTypeInfo const& StaticClass();\
	DECLARE_CLASS_TYPEINFO_(Class)

#define DECLARE_STI_NOBASE(Class) DECLARE_STI(Class, void)

#define DEFINE_CLASS_TYPEINFO_TEMPLATE(temp, Class, ...)\
	temp\
	ClassTypeInfo const& Class::GetTypeInfo() const\
	{                                  \
		return ::GetClassTypeInfo<Class>();\
	}\
	temp\
	ClassTypeInfo const& Class::StaticClass() \
	{                                        \
		return ::GetClassTypeInfo<Class>();\
	}\
	temp\
	ClassTypeInfoImpl<Class> const Class::TypeInfoHelper::s_TypeInfo = Class::TypeInfoHelper::MakeTypeInfo();\
	temp\
	ClassTypeInfoImpl<Class> Class::TypeInfoHelper::MakeTypeInfo() {\
		Name name = #Class;\
		ClassTypeInfo::Properties attrs;\
		auto const* parent = MaybeGetClassTypeInfo<Class::Super>();\
		EClassFlags classFlags {__VA_ARGS__};


#define DEFINE_CLASS_TYPEINFO(Class, ...) DEFINE_CLASS_TYPEINFO_(Class, Class::TypeInfoHelper, __VA_ARGS__)

#define DEFINE_CLASS_TYPEINFO_EXT(Class, ...) DEFINE_CLASS_TYPEINFO_(Class, TypeInfoHelper<Class>, __VA_ARGS__)

#define DEFINE_CLASS_TYPEINFO_(Class, Helper, ...)\
	ClassTypeInfo const& Class::GetTypeInfo() const\
	{                                  \
		return ::GetClassTypeInfo<Class>();\
	}\
	ClassTypeInfo const& Class::StaticClass() \
	{                                        \
		return ::GetClassTypeInfo<Class>();\
	}\
	ClassTypeInfoImpl<Class> const Helper::s_TypeInfo = Helper::MakeTypeInfo();\
	ClassTypeInfoImpl<Class> Helper::MakeTypeInfo() {\
		Name name = #Class;\
		ClassTypeInfo::Properties attrs;\
		auto const* parent = MaybeGetClassTypeInfo<Class::Super>();\
		EClassFlags classFlags {__VA_ARGS__};
	//= ConstructorHelper < std::function<void()>([] {\
	//g_TypeDB[TypeInfoHelper<Class>::ID] = std::make_unique<ClassTypeInfo>(#Class, GetTypeInfo<Class::Super>(), Vector<OwningPtr<AttributeInfo>> { 

#define BEGIN_REFL_PROPS()
#define REFL_PROP(prop, ...) attrs.emplace_back(#prop, ::GetTypeInfo<decltype(Type::prop)>(), offsetof(Type, prop), std::is_const_v<decltype(Type::prop)>, __VA_ARGS__);
#define REFL_PROP_SETTER(prop, setter, ...) attrs.emplace_back(#prop, ::GetTypeInfo<decltype(Type::prop)>(), offsetof(Type, prop), new MemberFunctionImpl<Type, decltype(std::declval<Type>().setter(Type{}.prop)), const decltype(Type::prop)&>(&Type::setter), std::is_const_v<decltype(Type::prop)>, __VA_ARGS__);
#define END_REFL_PROPS()
#define END_CLASS_TYPEINFO()                           \
		return ClassTypeInfoImpl<Type>{ name, parent, classFlags, std::move(attrs) }; \
	}
