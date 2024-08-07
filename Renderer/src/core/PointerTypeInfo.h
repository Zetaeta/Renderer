#pragma once

#include "core/TypeInfo.h"

class IAllocator
{
public:
	virtual void* Allocate(size_t size) = 0;
};

class NewAllocator : public IAllocator
{
	void* Allocate(size_t size) override {
		try
		{
			return new u8[size];
		}
		catch(...)
		{
			return nullptr;
		}
	}
};

static NewAllocator g_NewAllocator;

class PointerTypeInfo : public TypeInfo
{
public:
	enum EPtrFlags
	{
		NONE = 0,
		CONST_TARGET = 1,
		OWNING = 2
	};

	PointerTypeInfo(Name name, size_t size, ETypeFlags typeFlags, EPtrFlags flags, TypeInfo const& target)
		: TypeInfo(name, size, ETypeCategory::POINTER, typeFlags), m_TargetType(target) {}

	virtual bool		  IsNull(ConstReflectedValue ptr) const = 0;
	virtual ConstReflectedValue GetConst(ConstReflectedValue ptr) const = 0;
	virtual ReflectedValue	  Get(ConstReflectedValue ptr) const {
		RASSERT(!IsConst());
		return GetConst(ptr).ConstCast();
	}

	bool IsConst() const
	{
		return m_Flags & CONST_TARGET;
	}

	virtual bool New(ReflectedValue holder, TypeInfo const& newType) const = 0;

	// virtual void Reset(ValuePtr val) = 0;

	EPtrFlags GetFlags() const { return m_Flags; }

	EPtrFlags m_Flags;
	TypeInfo const& m_TargetType;
};

template<typename T>
struct TypeInfoHelper<std::unique_ptr<T>>
{
	constexpr static auto const NAME = concat("std::unique_ptr<", GetTypeInfoHelper<T>::NAME.str, ">");
	constexpr static u64 ID = crc64(NAME);

	constexpr static bool s_ConstTarget = std::is_const_v<T>;
	
	using UPtr = std::unique_ptr<T>;
	using Type = std::unique_ptr<T>;

	class UniquePtrTypeInfo : public PointerTypeInfo
	{
	public:
		UniquePtrTypeInfo()
			: PointerTypeInfo(NAME.str, sizeof(UPtr), ComputeFlags<Type>(), PointerTypeInfo::EPtrFlags((s_ConstTarget ? CONST_TARGET : NONE) | OWNING), GetTypeInfo<T>())
		{
		}

		bool IsNull(ConstReflectedValue ptr) const override
		{
			return ptr.GetAs<UPtr>() == nullptr;
		}

		ConstReflectedValue GetConst(ConstReflectedValue ptr) const override
		{
			RASSERT(ptr.GetType() == *this);
			return ConstReflectedValue::From ( **static_cast<UPtr const*>(ptr.GetPtr()) );
		}

		bool New(ReflectedValue holder, TypeInfo const& newType) const override
		{
			RASSERT(holder.GetType().IsDefaultConstructible());
			void* storage = new u8[newType.GetSize()];
			newType.Construct(storage);
			UPtr& uptr = holder.GetAs<UPtr>();
			uptr = UPtr { static_cast<T*>(storage) };
			return true;
		}

		void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override
		{
			RASSERT(false, "Can't copy unique ptr");
		}

		ReflectedValue Construct(void* location) const override
		{
			T* value = new (location) T;
			return ReflectedValue {value, this};
		}

		void Move(ReflectedValue const& from, ReflectedValue const& to) const override
		{
			RASSERT(from.GetType() == *this && to.GetType() == *this);
			to.GetAs<UPtr>() = std::move(from.GetAs<UPtr>());
		}
	};
	inline static UniquePtrTypeInfo const  s_TypeInfo;
};
