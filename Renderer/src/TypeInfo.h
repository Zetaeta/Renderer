#pragma once

#include "Types.h"
#include "Utils.h"
#include <unordered_map>
#include <memory>

using TypeID = u64;

enum class ETypeCategory : u8
{
	BASIC,
	CLASS,
	CONTAINER,
	POINTER
};

class TypeInfo;

template<bool IsConst>
class TValuePtr
{
public:
	TypeInfo const& GetType()
	{
		return *m_Type;
	}

	ApplyConst<void, IsConst>* GetObject() const
	{
		return m_Obj;
	}

	template<typename T>
	ApplyConst<T, IsConst>& GetAs();

protected:
	ApplyConst<void, IsConst>* m_Obj;
	TypeInfo const* m_Type;
};

using ValuePtr = TValuePtr<false>;
using ConstValuePtr = TValuePtr<true>;


class TypeInfo
{
public:
	TypeInfo(String name)
		: m_Name(name), m_Category(ETypeCategory::BASIC) {}

	Name const& GetName() { return m_Name; }

	ETypeCategory GetTypeCategory() const
	{
		return m_Category;
	}

	bool operator==(TypeInfo const& other)
	{
		return this == &other;
	}

	virtual void Copy(ConstValuePtr const& from, ValuePtr const& to) {};

	virtual bool Contains(TypeInfo const& cls) {
		return cls == *this;
	}

private:
	Name m_Name;
	ETypeCategory m_Category;
};

extern std::unordered_map<TypeID, std::unique_ptr<TypeInfo>> g_TypeDB;

class AttrAccessor;

class AttributeInfo
{
	Name m_Name;
	TypeInfo* m_Type;
	AttrAccessor* m_Accessor;
//	void
};

class ClassTypeInfo : public TypeInfo
{
public:

	virtual bool Contains(TypeInfo const& cls) {
		return cls.GetTypeCategory() == ETypeCategory::CLASS && InheritsFrom(static_cast<ClassTypeInfo const&>(cls));
	}

	bool InheritsFrom(ClassTypeInfo const& cls) const;

	template<typename T>
	bool IsA()
	{
		return InheritsFrom(GetTypeInfo<T>());
	}

	ClassTypeInfo const* GetParent() const { return m_Parent; }

protected:
	ClassTypeInfo const* m_Parent;
	Vector<AttributeInfo> m_Attrs;

public:
};

template<typename TClass>
class ClassTypeInfoImpl : public ClassTypeInfo
{
	void Copy(ConstValuePtr from, ValuePtr to)
	{
		RASSERT(from.GetType().InheritsFrom(*this) && to.GetType() == *this, "Incompatible types");
		to.GetAs<TClass>() = from.GetAs<TClass>();
	}
};


template<bool IsConst>
class TClassValuePtr : public TValuePtr<IsConst>
{
public:
	ClassTypeInfo const& GetType()
	{
		//return *static_cast<ClassTypeInfo const*>(m_Type);
		return static_cast<ClassTypeInfo const&>(TValuePtr::GetType());
	}
};


using ClassValuePtr = TClassValuePtr<false>;
using ConstClassValuePtr = TClassValuePtr<true>;

class AttrAccessor
{
	virtual ValuePtr Access(ClassValuePtr const& obj);
	virtual ConstValuePtr Access(ConstClassValuePtr const& obj);
};

template<typename TParent, typename TChild>
class AttrAccessorImpl
{
	TParent::TChild* m_Member;
	virtual ValuePtr Access(ClassValuePtr const& obj)
	{
		RASSERT(obj.GetType().IsA<TParent>());
	}
	virtual ConstValuePtr Access(ConstClassValuePtr const& obj);
};


template<typename T>
struct TypeInfoFinder
{
	
};

template<typename T>
TypeInfo GetTypeInfo()
{
	return TypeInfo {typeid(T).name() };
}

//template<class T>
//std::enable_if_t<std::is_class_v<T>, ClassTypeInfo const&> GetClassTypeInfo()
//{
//	
//}

template<typename T>
String GetTypeName()
{
	return GetTypeInfo<T>().GetName();
}

template<typename TBasic>
class BasicTypeInfo : public TypeInfo
{
	void Copy(ConstValuePtr from, ValuePtr to)
	{
		assert(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<TBasic>() = from.GetAs<TBasic>();
	}
};

#include "TypeInfo.inl"

