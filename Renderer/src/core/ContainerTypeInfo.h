#pragma once

#include "core/TypeInfo.h"
#include "core/ClassTypeInfo.h"

class ContainerAccessor
{
public:
	virtual void Resize(size_t size) = 0;
	virtual ReflectedValue GetAt(size_t pos) = 0;
	virtual size_t			  GetSize() const = 0;
};

class ConstContainerAccessor
{
public:
	virtual ConstReflectedValue GetAt(size_t pos) = 0;
	virtual size_t			  GetSize() const = 0;
};

class ContainerTypeInfo : public TypeInfo
{
public:
	enum EContainerFlag
	{
		NONE = 0,
		RESIZABLE = 1,
		CONST_CONTENT = 2
	};

	ContainerTypeInfo(Name name, size_t size, ETypeFlags typeFlags, EContainerFlag flags, TypeInfo const& contained,
						MemberMetadata&& containedMeta)
		: TypeInfo(name, size, ETypeCategory::CONTAINER, typeFlags), m_Flags(flags)
		, m_ContainedType(contained), m_ContainedMeta(std::move(containedMeta)) {}

	virtual OwningPtr<ContainerAccessor>	  CreateAccessor(ReflectedValue container) const = 0;
	virtual OwningPtr<ConstContainerAccessor> CreateAccessor(ConstReflectedValue container) const = 0;

	EContainerFlag GetFlags() const
	{
		return m_Flags;
	}

	bool HasFlag(EContainerFlag flag) const
	{
		return m_Flags & flag;
	}

	TypeInfo const& GetContainedType() const
	{
		return m_ContainedType;
	}
	
	MemberMetadata const& GetContainedMeta() const
	{
		return m_ContainedMeta;
	}

protected:
	MemberMetadata m_ContainedMeta;
	EContainerFlag m_Flags = NONE;
	TypeInfo const& m_ContainedType;
};

template<typename T, typename TContained, typename TAccessor, typename TConstAccessor>
class ContainerTypeInfoImpl : public ContainerTypeInfo
{
public:
	ContainerTypeInfoImpl(Name name, size_t size, EContainerFlag flags, MemberMetadata&& containedMeta = {})
		: ContainerTypeInfo(name, size, ComputeFlags<T>(), flags, GetTypeInfo<TContained>(), std::move(containedMeta))
	{ }

	OwningPtr<ContainerAccessor> CreateAccessor(ReflectedValue container) const override
	{
		ZE_ASSERT(container.GetType() == *this);
		return std::make_unique<TAccessor>(container);
	}

	OwningPtr<ConstContainerAccessor> CreateAccessor(ConstReflectedValue container) const override
	{
		ZE_ASSERT(container.GetType() == *this);
		return std::make_unique<TConstAccessor>(container);
	}

	void Copy(ConstReflectedValue const& from, ReflectedValue const& to) const override
	{
		if constexpr (std::copyable<T> && std::copyable<TContained>)
		{
			assert(from.GetType() == *this && to.GetType() == *this);
			to.GetAs<T>() = from.GetAs<T>();
		}
		else
		{
			ZE_ASSERT(false, "Not copyable");
		}
	}

	ReflectedValue Construct(void* location) const override
	{
		T* value = new (location) T;
		return ReflectedValue {value, this};
	}

	void Move(ReflectedValue const& from, ReflectedValue const& to) const override
	{
		if constexpr (std::movable<T>)
		{
			assert(from.GetType() == *this && to.GetType() == *this);
			to.GetAs<T>() = std::move(from.GetAs<T>());
		}
		else
		{
			ZE_ASSERT(false, "Not movable");
		}
	}
};

template<u32 N, typename T, glm::qualifier Q>
struct VecNames
{
	constexpr static auto const NAME = concat("vec<", U32ToStr<N>::value, GetTypeName<T>(), U32ToStr<static_cast<u32>(Q)>::value, ">");
};

template<u32 N>
struct VecNames<N, float, defaultp>
{
	constexpr static auto const NAME = concat("vec", U32ToStr<N>::value);
};

#define DECLARE_AGGREGATE_CONTAINER_TYPEINFO(type, entryType, ...)\
	template<>\
	struct TypeInfoHelper<type>\
	{\
		constexpr static auto const NAME = Static(#type);\
		constexpr static u64 ID = crc64(NAME);\
		using Type = type;\
		template<typename TParent, bool IsConst>\
		class TAggAccessor : public TParent\
		{\
		public:\
			using CType = ApplyConst<type, IsConst>;\
			using CEntry = ApplyConst<entryType, IsConst>;\
			TAggAccessor(CType& v)\
				: m_Start(reinterpret_cast<CEntry*>(&v)) {}\
\
			TAggAccessor(TReflectedValue<IsConst>& v)\
				: TAggAccessor(v.GetAs<Type>()) {}\
				\
			constexpr static size_t SIZE = sizeof(type) / sizeof(entryType);\
\
			TReflectedValue<IsConst> GetAt(size_t pos) override\
			{\
				ZE_ASSERT (pos < SIZE);\
				return TReflectedValue<IsConst>::From(m_Start[pos]);\
			}\
\
			size_t GetSize() const override\
			{\
				return SIZE;\
			}\
\
			void Resize(size_t size)\
			{\
				ZE_ASSERT (false);\
			}\
			\
			CEntry* m_Start;\
		};\
		using AggAccessor = TAggAccessor<ContainerAccessor, false>;\
		using ConstAggAccessor = TAggAccessor<ConstContainerAccessor, true>;\
		inline static ContainerTypeInfoImpl<Type, entryType, AggAccessor, ConstAggAccessor> const s_TypeInfo{ NAME.str, sizeof(Type), ContainerTypeInfo::NONE, __VA_ARGS__ };\
	}

template<u32 N, typename T, glm::qualifier Q>
struct TypeInfoHelper<vec<N,T,Q>>
{
	constexpr static auto const NAME = VecNames<N,T,Q>::NAME;
	constexpr static u64 ID = crc64(NAME);

	using Type = vec<N,T,Q>;
	
	template<typename TParent, bool IsConst>
	class TVecAccessor : public TParent
	{
	public:
		using cvec = ApplyConst<vec<N,T,Q>, IsConst>;
		TVecAccessor(vec3& v)
			: m_Vec(v)
		{
		}

		TVecAccessor(TReflectedValue<IsConst>& v)
			: m_Vec(v.GetAs<Type>())
		{
		}

		TReflectedValue<IsConst> GetAt(size_t pos) override
		{
			ZE_ASSERT(pos < N);
			return TReflectedValue<IsConst>::From(m_Vec[NumCast<glm::length_t>(pos)]);
		}

		size_t GetSize() const override
		{
			return m_Vec.length();
		}
		//ApplyConst<vec3, IsConst>& m_Vec;

		void Resize(size_t size)
		{
			ZE_ASSERT(false);
		}
		
		cvec& m_Vec;
	};

	using VecAccessor = TVecAccessor<ContainerAccessor, false>;
	using ConstVecAccessor = TVecAccessor<ConstContainerAccessor, true>;
			
	inline static ContainerTypeInfoImpl<Type, T, VecAccessor, ConstVecAccessor> const s_TypeInfo{ NAME.str, sizeof(Type), ContainerTypeInfo::NONE };
};

template<typename TEntry>
struct TypeInfoHelper<Vector<TEntry>>
{
	constexpr static auto NAME = concat("std::vector<", TypeInfoHelper<TEntry>::NAME.str, ">");
	constexpr static u64 ID = crc64(NAME);
	using Type = Vector<TEntry>;

	class ConstVectorAccessor : public ConstContainerAccessor
	{
	public:
		ConstVectorAccessor(ConstReflectedValue container)
			: m_Vec(container.GetAs<Type>())
		{
		}

		ConstReflectedValue GetAt(size_t pos)
		{
			ZE_ASSERT(pos < m_Vec.size());
			return ConstReflectedValue::From(m_Vec[pos]);
		}

		size_t GetSize() const
		{
			return m_Vec.size();
		}

		Type const& m_Vec;
	};

	class VectorAccessor : public ContainerAccessor
	{
	public:
		VectorAccessor(ReflectedValue container)
			: m_Vec(container.GetAs<Type>())
		{
		}

		void Resize(size_t size)
		{
			m_Vec.resize(size);
		}
		
		ReflectedValue GetAt(size_t pos)
		{
			ZE_ASSERT(pos < m_Vec.size());
			return ReflectedValue::From(m_Vec[pos]);
		}

		size_t GetSize() const
		{
			return m_Vec.size();
		}

		Type& m_Vec;
	};
	inline static ContainerTypeInfoImpl<Type, TEntry, VectorAccessor, ConstVectorAccessor> const s_TypeInfo{ NAME.str, sizeof(Type), ContainerTypeInfo::RESIZABLE };
};	

