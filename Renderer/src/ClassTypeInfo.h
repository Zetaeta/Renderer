#pragma once
#include "TypeInfo.h"
#include <variant>
#include <array>
#include <optional>


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
	RCOPY_PROTECT(MemberMetadata);
public:
	RMOVE_DEFAULT(MemberMetadata);

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
		RASSERT(!IsSet(datum.m_Type));

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
	MemberMetadata m_Meta;

	PropertyInfo(Name name, TypeInfo const& type, MemberMetadata&& meta, bool isConst = false)
		: m_Name(name), m_Type(&type), m_Const(isConst), m_Meta(std::move(meta)) {}

public:
	Name GetName() const { return m_Name; }
	TypeInfo const&		  GetType() const { return *m_Type; }
	bool				  IsConst() const { return m_Const; }
	virtual ValuePtr Access(ClassValuePtr const& obj) const = 0;
	virtual ConstValuePtr Access(ConstClassValuePtr const& obj) const = 0;
	MemberMetadata const& GetMetadata() const { return m_Meta; }
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
	AttrAccessorImpl(Name name, MemberPtr<TParent, TChild> member, MemberMetadata&& meta = {}) :PropertyInfo(name, GetTypeInfo<TChild>(), std::move(meta)), m_Member(member) {}
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
#define REFL_PROP(prop, ...) attrs.emplace_back(std::make_unique < AttrAccessorImpl<Type, decltype(Type::prop)>>(#prop, &Type::prop, __VA_ARGS__));
#define END_REFL_PROPS()
#define END_CLASS_TYPEINFO()                           \
	return ClassTypeInfoImpl<Type>{ name, parent, std::move(attrs) }; \
	}
