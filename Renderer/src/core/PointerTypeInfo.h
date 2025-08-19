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
		ZE_ASSERT(!IsConst());
		return GetConst(ptr).ConstCast();
	}

	bool IsConst() const
	{
		return m_Flags & CONST_TARGET;
	}

	TypeInfo const& GetTargetType() const
	{
		return m_TargetType;
	}

	virtual bool New(ReflectedValue holder, TypeInfo const& newType) const = 0;

	// virtual void Reset(ValuePtr val) = 0;

	EPtrFlags GetFlags() const { return m_Flags; }

	EPtrFlags m_Flags;
	TypeInfo const& m_TargetType;
};

template<typename TPtr>
class TPtrTypeInfo : public PointerTypeInfo
{
	using T = std::remove_reference_t<decltype(*std::declval<TPtr>())>;
	constexpr static bool s_ConstTarget = std::is_const_v<T>;
public:
	TPtrTypeInfo()
		: PointerTypeInfo(TypeInfoHelper<TPtr>::NAME.str, sizeof(TPtr), ComputeFlags<TPtr>(), PointerTypeInfo::EPtrFlags((s_ConstTarget ? CONST_TARGET : NONE) | OWNING), GetTypeInfo<T>())
	{
	}

	bool IsNull(ConstReflectedValue ptr) const override
	{
		return ptr.GetAs<TPtr>() == nullptr;
	}

	ConstReflectedValue GetConst(ConstReflectedValue ptr) const override
	{
		ZE_ASSERT(ptr.GetType() == *this);
		return ConstReflectedValue::From ( **static_cast<TPtr const*>(ptr.GetPtr()) );
	}

	bool New(ReflectedValue holder, TypeInfo const& newType) const override
	{
		ZE_ASSERT(holder.GetType().IsDefaultConstructible());
		void* storage = new u8[newType.GetSize()];
		newType.Construct(storage);
		TPtr& uptr = holder.GetAs<TPtr>();
		uptr = TPtr { static_cast<T*>(storage) };
		return true;
	}

	void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override
	{
		if constexpr (std::is_copy_assignable_v<TPtr>)
		{
			to.GetAs<TPtr>() = from.GetAs<TPtr>();
		}
		else
		{
			ZE_ASSERT(false, "Can't copy %s", TypeInfoHelper<T>::Name.str);
		}
	}

	ReflectedValue Construct(void* location) const override
	{
		T* value = new (location) T;
		return ReflectedValue {value, this};
	}

	void Move(ReflectedValue const& from, ReflectedValue const& to) const override
	{
		ZE_ASSERT(from.GetType() == *this && to.GetType() == *this);
		to.GetAs<TPtr>() = std::move(from.GetAs<TPtr>());
	}
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
			ZE_ASSERT(ptr.GetType() == *this);
			return ConstReflectedValue::From ( **static_cast<UPtr const*>(ptr.GetPtr()) );
		}

		bool New(ReflectedValue holder, TypeInfo const& newType) const override
		{
			ZE_ASSERT(holder.GetType().IsDefaultConstructible());
			void* storage = new u8[newType.GetSize()];
			newType.Construct(storage);
			UPtr& uptr = holder.GetAs<UPtr>();
			uptr = UPtr { static_cast<T*>(storage) };
			return true;
		}

		void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override
		{
			ZE_ASSERT(false, "Can't copy unique ptr");
		}

		ReflectedValue Construct(void* location) const override
		{
			T* value = new (location) T;
			return ReflectedValue {value, this};
		}

		void Move(ReflectedValue const& from, ReflectedValue const& to) const override
		{
			ZE_ASSERT(from.GetType() == *this && to.GetType() == *this);
			to.GetAs<UPtr>() = std::move(from.GetAs<UPtr>());
		}
	};
	inline static UniquePtrTypeInfo const  s_TypeInfo;
};

template<typename T>
struct TypeInfoHelper<std::shared_ptr<T>>
{
	constexpr static auto const NAME = concat("std::shared_ptr<", GetTypeInfoHelper<T>::NAME.str, ">");
	constexpr static u64 ID = crc64(NAME);

	constexpr static bool s_ConstTarget = std::is_const_v<T>;
	
	using Type = std::shared_ptr<T>;

	inline static TPtrTypeInfo<Type> const  s_TypeInfo;
};
